// Fill out your copyright notice in the Description page of Project Settings.


#include "VRPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "VRGameModeBase.h"
#include "EnhancedInputComponent.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "DrawDebugHelpers.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Components/WidgetInteractionComponent.h"
#include "Haptics/HapticFeedbackEffect_Curve.h"

// Sets default values
AVRPlayer::AVRPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	vrCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
	vrCamera->SetupAttachment(RootComponent);
	vrCamera->bUsePawnControlRotation = false;

	leftHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("leftHand"));
	leftHand->SetupAttachment(RootComponent);
	leftHand->SetTrackingMotionSource(FName("Left"));
	rightHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("rightHand"));
	rightHand->SetupAttachment(RootComponent);
	rightHand->SetTrackingMotionSource(FName("Right"));

	// ���̷�Ż ������Ʈ �����
	leftHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("leftHandMesh"));
	leftHandMesh->SetupAttachment(leftHand);
	rightHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("rightHandMesh"));
	rightHandMesh->SetupAttachment(rightHand);

	// ���̷�Ż �޽� �ε��ؼ� �Ҵ�
	ConstructorHelpers::FObjectFinder<USkeletalMesh> tempMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/MannequinsXR/Meshes/SKM_MannyXR_left.SKM_MannyXR_left'"));
	if(tempMesh.Succeeded())
	{
		leftHandMesh->SetSkeletalMesh(tempMesh.Object);
		leftHandMesh->SetRelativeLocation(FVector(-2.9f, -3.5f, 4.5f));
		leftHandMesh->SetRelativeRotation(FRotator(-25.0f, -179.9f, 89.9f));
	}
	ConstructorHelpers::FObjectFinder<USkeletalMesh> tempMesh1(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/MannequinsXR/Meshes/SKM_MannyXR_right.SKM_MannyXR_right'"));
	if (tempMesh1.Succeeded())
	{
		rightHandMesh->SetSkeletalMesh(tempMesh1.Object);
		rightHandMesh->SetRelativeLocation(FVector(-2.9f, 3.5f, 4.5f));
		rightHandMesh->SetRelativeRotation(FRotator(25.0f, 0.0f, 89.9f));
	}

	// teleport
	teleportCircle = CreateDefaultSubobject<UNiagaraComponent>(TEXT("teleportCircle"));
	teleportCircle->SetupAttachment(RootComponent);
	teleportCircle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	teleportCurveComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("teleportCurveComp"));
	teleportCurveComp->SetupAttachment(RootComponent);

	// ���Լհ���
	rightAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("rightArm"));
	rightAim->SetupAttachment(RootComponent);
	rightAim->SetTrackingMotionSource(FName("RightAim"));

	// widget
	widgetInteractionComp = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("widgetInteractionComp"));
	widgetInteractionComp->SetupAttachment(rightAim);
}

// Called when the game starts or when spawned
void AVRPlayer::BeginPlay()
{
	Super::BeginPlay();

	// enhanced input ���ó��
	auto PC = Cast<APlayerController>(GetWorld()->GetFirstPlayerController());

	if(PC)
	{
		auto localPlayer = PC->GetLocalPlayer();

		auto subSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(localPlayer);

		if(subSystem)
		{
			subSystem->AddMappingContext(IMC_VRInput, 0);
			subSystem->AddMappingContext(IMC_Hand, 0);
		}
	}

	ResetTeleport();

	if (crossHairFactory)
	{
		crossHair = GetWorld()->SpawnActor<AActor>(crossHairFactory);
	}

	// hmd�� ���� �Ǿ� ���� �ʴٸ�
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() == false)
	{
		// ���� �׽�Ʈ �� �� �ִ� ��ġ�� �̵���Ű��
		rightHand->SetRelativeLocation(FVector(20.0f, 20.0f, 0.0f));
		rightAim->SetRelativeLocation(FVector(20.0f, 20.0f, 0.0f));

		// ī�޶��� use pawn control rotation Ȱ��ȭ
		vrCamera->bUsePawnControlRotation = true;
	}
	// ���࿡ hmd�� ����Ǿ� �ִٸ�
	else
	{
		// -> �⺻ Ʈ��ŷ offset ����
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye);
	}
}

// Called every frame
void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// HMD �� ����Ǿ� ���� ������
	if(UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() == false)
	{
		// -> ���� ī�޶� ����� ��ġ�ϵ��� �Ѵ�
		rightHand->SetRelativeRotation(vrCamera->GetRelativeRotation());
		rightAim->SetRelativeRotation(vrCamera->GetRelativeRotation());
	}

	// corsshair
	DrawCrossHair();

	// �ڷ���Ʈ Ȯ�� ó��
	if(bTeleporting)
	{
		// ���� ������ �׸��ٸ�
		if(bTeleportCurve == false)
		{
			DrawTeleportStraight();
		}
		// �׷��� ������
		else
		{
			// � �׸���
			DrawTeleportCurve();
		}

		// ���̾ư��� �̿��ؼ� ���׸���
		if (teleportCurveComp != nullptr)
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(teleportCurveComp, FName("User.PointArray"), lines);
		}
	}

	Grabbing();
}

// Called to bind functionality to input
void AVRPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto inputSystem = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	if(inputSystem)
	{
		// binding move
		inputSystem->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AVRPlayer::Move);
		// binding mouse
		inputSystem->BindAction(IA_Mouse, ETriggerEvent::Triggered, this, &AVRPlayer::Turn);
		// binding teleport
		inputSystem->BindAction(IA_Teleport, ETriggerEvent::Started, this, &AVRPlayer::TeleportStart);
		inputSystem->BindAction(IA_Teleport, ETriggerEvent::Completed, this, &AVRPlayer::TeleportEnd);
		inputSystem->BindAction(IA_Fire, ETriggerEvent::Started, this, &AVRPlayer::FireInput);
		inputSystem->BindAction(IA_Fire, ETriggerEvent::Completed, this, &AVRPlayer::ReleasedUIInput);
		// ���
		inputSystem->BindAction(IA_Grab, ETriggerEvent::Started, this, &AVRPlayer::TryGrab);
		inputSystem->BindAction(IA_Grab, ETriggerEvent::Completed, this, &AVRPlayer::UnTryGrab);
	}
}

void AVRPlayer::Move(const FInputActionValue& Values)
{
	// 1. ������� �Է¿� ����
	FVector2D Axis = Values.Get<FVector2D>();

	AddMovementInput(GetActorForwardVector(), Axis.X);

	AddMovementInput(GetActorRightVector(), Axis.Y);

	// 2. �յ� �¿��� ������ �ʿ�
	//FVector dir = FVector(Axis.X, Axis.Y, 0);

	// 3. �̵��ϰ� �ʹ�
	//FVector p = GetActorLocation();

	//FVector vt = dir * moveSpeed * GetWorld()->GetDeltaSeconds();

	//SetActorLocation(p  + vt);
}

void AVRPlayer::Turn(const FInputActionValue& Values)
{
	FVector2D Axis = Values.Get<FVector2D>();

	AddControllerYawInput(Axis.X);
	
	AddControllerPitchInput(Axis.Y);
}

// �ڷ���Ʈ ��� Ȱ��ȭ
void AVRPlayer::TeleportStart(const FInputActionValue& Values)
{
	// �������� ����ڰ� ��� ����Ű���� �ֽ��ϰ� �ʹ�
	bTeleporting = true;

	// ������ ���̵��� Ȱ��ȭ
	teleportCurveComp->SetVisibility(true);
}

void AVRPlayer::TeleportEnd(const FInputActionValue& Values)
{
	// �ڷ���Ʈ ��� ����
	// ���� �ڷ���Ʈ�� �Ұ����ϴٸ�
	// ���� ó���� ���� �ʴ´�
	if(ResetTeleport() == false)
	{
		return;
	}

	// ���� ���� ���� ó���ϰ�
	if (bIsWarp)
	{
		DoWarp();
	}
	// �׷��� ������
	else
	{
		// �ڷ���Ʈ ��ġ�� �̵��ϰ� �ʹ�
		SetActorLocation(teleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
}

bool AVRPlayer::ResetTeleport()
{
	// �ڷ���Ʈ ��Ŭ�� ���϶��� �ڷ���Ʈ ����
	bool bCanTeleport = teleportCircle->GetVisibleFlag();

	// ��Ŭ �Ⱥ��̰� ó��
	teleportCircle->SetVisibility(false);
	teleportLocation = teleportCircle->GetComponentLocation();
	bTeleporting = false;
	teleportCurveComp->SetVisibility(false);

	return bCanTeleport;
}

void AVRPlayer::DrawTeleportStraight()
{
	lines.RemoveAt(0, lines.Num());
	// ������ �׸��� �ʹ�.
	// �ʿ����� : ������, ������
	FVector startPos = rightAim->GetComponentLocation();
	FVector endPos = startPos + rightAim->GetForwardVector() * 1000.0f;

	// �� �� ���̿� �浹�� �Ǵ��� üũ
	CheckHitTeleport(startPos, endPos);

	lines.Add(startPos);
	lines.Add(endPos);
	//DrawDebugLine(GetWorld(), startPos, endPos, FColor::Red, false, -1, 0, 1);
}

bool AVRPlayer::CheckHitTeleport(FVector lastPos, FVector& curPos)
{
	FHitResult result;

	bool bHit = HitTest(lastPos, curPos, result);

	if (bHit && result.GetActor()->GetName().Contains(TEXT("Floor")))
	{
		// ������ ���� ���� ������ ����
		curPos = result.Location;
		// ��Ŭ Ȱ��ȭ
		teleportCircle->SetVisibility(true);
		// �ڷ���Ʈ ��Ŭ�� ��ġ
		teleportCircle->SetWorldLocation(curPos);
	}
	else
	{
		teleportCircle->SetVisibility(false);
	}
	return bHit;
}

bool AVRPlayer::HitTest(FVector lastPos, FVector curPos, FHitResult& result)
{
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);
	bool bHit = GetWorld()->LineTraceSingleByChannel(result, lastPos, curPos, ECollisionChannel::ECC_Visibility, params);
	return bHit;
}

// �־��� �ӵ��� ����ü�� ���������� ����ü�� ������ ���� ���
void AVRPlayer::DrawTeleportCurve()
{
	// lines �ʱ�ȭ
	lines.RemoveAt(0, lines.Num());
	// 4. ������, ����, �� ���� ����ü�� ������
	FVector pos = rightAim->GetComponentLocation();
	FVector dir = rightAim->GetForwardVector() * curvedPower;

	// �������� ���� ���� ���
	lines.Add(pos);

	for (int i = 0; i < lineSmooth; i++)
	{
		// ���� �� ���
		FVector lastPos = pos;
		// 3. ����ü�� �̵������ϱ�(�ݺ�������)
		// v = v0 +at
		dir += FVector::UpVector * gravity * simulratedTime;
		// p = p0 +vt
		pos += dir * simulratedTime;
		// 2. ����ü�� ��ġ����
		// -> ���� �� ���̿� ��ü�� ���θ��� �ִٸ�
		if (CheckHitTeleport(lastPos, pos))
		{
			//     -> �� ���� ������ ������ ����
			lines.Add(pos);
			break;
		}
		// 1. ���� �������
		lines.Add(pos);
	}

	//for(int i = 0; i<lines.Num() - 1; i++)
	//{
		//� �׸���
	//	DrawDebugLine(GetWorld(), lines[i], lines[i + 1], FColor::Red, false, -1, 0, 1);
	//}
}

void AVRPlayer::DoWarp()
{
	// ���� ����� Ȱ��ȭ �Ǿ� ������
	if (bIsWarp != true)
	{
		return;
	}
	// ���� ó�� �ϰ� �ʹ�
	// ->���� �ð����� ������ �̵�
	// ��� �ð��� �ʱ�ȭ
	curTime = 0;
	// �ø��� ��Ȱ��ȭ
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 3. �ð��� �귯���Ѵ�
	// 2. �����ð�����
	// [ĸ��] ( ) -> { body }
	GetWorld()->GetTimerManager().SetTimer(warpHandle, FTimerDelegate::CreateLambda(
		[this]()->void
		{
			// body
			// ���� �ð��ȿ� �������� ����
			// 3. �ð��� �帥��
			curTime += GetWorld()->DeltaTimeSeconds;
			// ����
			FVector curPos = GetActorLocation();
			// ����
			FVector endPos = teleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			// 2. �̵��Ѵ�
			curPos = FMath::Lerp<FVector>(curPos, endPos, curTime / warpTime);
			// 1. �������� �����Ѵ�
			SetActorLocation(curPos);

			// �ð��� �� �귶�ٸ�
			if (curTime >= warpTime)
			{
				// ->�� ��ġ�� �Ҵ��Ѵ�
				SetActorLocation(endPos);
				// ->Ÿ�̸� ����(���� ����)
				GetWorld()->GetTimerManager().ClearTimer(warpHandle);

				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
			// �Ÿ��� ���� ����� ���� �� ��ġ�� �Ҵ����ֱ�
			// _________________________��Ʈ ����� ������___________________________
			//float dis = FVector::Dist(curPos, endPos);
			//if (dis < 0.1f)
			//{
			//	curPos = endPos;
			//}

		}
	), 0.02f, true);
}

void AVRPlayer::ReleasedUIInput()
{
	if (widgetInteractionComp)
	{
		widgetInteractionComp->ReleasePointerKey(FKey(FName("LeftMouseButton")));
	}
}

void AVRPlayer::FireInput(const FInputActionValue& Values)
{
	// UI�� �̺�Ʈ ����
	if (widgetInteractionComp)
	{
		widgetInteractionComp->PressPointerKey(FKey(FName("LeftMouseButton")));
		//widgetInteractionComp->PressPointerKey(FKey(FName("OculusTouch(R)Trigger")));
	}

	// ����ó��
	auto PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->PlayHapticEffect(HF_Fire, EControllerHand::Right);
	}

	// linetrace �̿��ؼ� ���� ��� �ʹ�
	// ������
	FVector startPos = rightAim->GetComponentLocation();
	// ������
	FVector endPos = startPos + rightAim->GetForwardVector() * 10000;
	// �ѽ��(linetrace ����)
	FHitResult hitInfo;
	bool bHit = HitTest(startPos, endPos, hitInfo);
	// ���� �ε��� ���� �ִٸ� ������
	if (bHit)
	{
		auto hitComp = hitInfo.GetComponent();
		if (hitComp && hitComp->IsSimulatingPhysics())
		{
			// ����������
			hitComp->AddForceAtLocation(hitComp->GetMass() * (endPos - startPos).GetSafeNormal() * 100000.0f, hitInfo.Location);
		}
	}
}

// �Ÿ��� ���� ũ�ν���� ũ�Ⱑ ���� ���̵��� ����
void AVRPlayer::DrawCrossHair()
{
	// ������
	FVector startPos = rightAim->GetComponentLocation();
	// ����
	FVector endPos = startPos + rightAim->GetForwardVector() * 10000.0f;
	// �浹 ������ ����
	FHitResult hitInfo;
	// �浹 üũ
	bool bHit = HitTest(startPos, endPos, hitInfo);

	float distance = 0;

	if (bHit)
	{
		// -> �浹�� �߻��ϸ� �浹�� ������ ũ�ν���� ǥ��
		crossHair->SetActorLocation(hitInfo.Location);
		distance = hitInfo.Distance;
	}
	// �׷��� ������
	else
	{
		// -> �׳� ������ ũ�ν���� ǥ��
		crossHair->SetActorLocation(endPos);
		distance = (endPos - startPos).Size();
	}

	crossHair->SetActorScale3D(FVector(FMath::Max<float>(1, distance)));

	// ������
	// ->ũ�ν��� ī�޶� �ٶ󺸵��� ó��
	FVector direction = crossHair->GetActorLocation() - vrCamera->GetComponentLocation();
	crossHair->SetActorRotation(direction.Rotation());
}

// ��ü�� ��� �ʹ�
void AVRPlayer::TryGrab()
{
	// �߽���
	FVector center = rightHand->GetComponentLocation();

	// �浹üũ(�� �浹)
	// �浹�� ��ü�� ����� �迭
	TArray<FOverlapResult> hitObjs;
	// �浹 ���� �ۼ�
	FCollisionQueryParams param;
	param.AddIgnoredActor(this);
	param.AddIgnoredComponent(rightHand);

	bool bHit = GetWorld()->OverlapMultiByChannel(hitObjs, center, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(GrabRange), param);

	// �浹 ���� �ʾҴٸ� �ƹ� ó�� ���� �ʴ´�
	if (bHit == false)
	{
		return;
	}
	// -> ���� ����� ��ü �⵵�� �Ѵ� (�������)
	// ���� ����� ��ü �ε���
	int closest = 0;

	for(int i = 0; i<hitObjs.Num(); i++)
	{
		// 1. ���� ����� Ȱ��ȭ �Ǿ� �ִ� �༮�� �Ǵ�
		// -> ���� �ε��� ������Ʈ�� ��������� ��Ȱ��ȭ �Ǿ� �ִٸ�
		if (hitObjs[i].GetComponent()->IsSimulatingPhysics() == false)
		{
			// �����ϰ� ���� �ʴ�
			continue;
		}
		// ��Ҵ�
		bIsGrabbed = true;
		// 2. �հ� ���� ����� �༮�� �̹��� ������ �༮�� �� ����� �༮�� �ִٸ�
		// -> �ʿ�Ӽ� : ���� ���� ����� �༮�� �հ��� �Ÿ�
		float closestDist = FVector::Dist(hitObjs[closest].GetActor()->GetActorLocation(), center);
		// -> �ʿ�Ӽ� : �̹��� ������ �༮�� �հ��� �Ÿ�
		float nextDist = FVector::Dist(hitObjs[i].GetActor()->GetActorLocation(), center);
		// 3. ���� �̹����� ���粨 ���� �� �����ٸ�
		if (nextDist < closestDist)
		{
			// -> ���� ����� �༮���� �����ϱ�
			closest = i;
		}
	}

	// ���� ��Ҵٸ�
	if (bIsGrabbed)
	{
		grabbedObject = hitObjs[closest].GetComponent();
		// -> ��ü ������� ��Ȱ��ȭ
		grabbedObject->SetSimulatePhysics(false);
		grabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// -> �տ� �ٿ�����
		grabbedObject->AttachToComponent(rightHand, FAttachmentTransformRules::KeepWorldTransform);

		prevPos = rightHand->GetComponentLocation();
	}
}

// ���� �༮�� ������ ���� �ʹ�
void AVRPlayer::UnTryGrab()
{
	if (bIsGrabbed == false)
	{
		return;
	}

	// 1. ���� ���� ���·� ��ȭ
	bIsGrabbed = false;
	// 2. �տ��� �����
	grabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	// 3. ������� Ȱ��ȭ
	grabbedObject->SetSimulatePhysics(true);
	// 4. �浹��� Ȱ��ȭ
	grabbedObject->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// ������
	grabbedObject->AddForce(throwDirection * throwPower * grabbedObject->GetMass());

	// ȸ����Ű��
	// ���ӵ� = (1 / dt) * dTheta(Ư�� �� ���� ���� ���� axis, angle)
	float angle;
	FVector axis;
	deltaRotation.ToAxisAndAngle(axis, angle);
	float dt = GetWorld()->GetDeltaSeconds();
	FVector angularVelocity = (1.0f / dt) * angle * axis;
	grabbedObject->SetPhysicsAngularVelocityInRadians(angularVelocity * toquePower, true);

	grabbedObject = nullptr;
}

// ���� ������ ������Ʈ �ϱ� ���� ���
void AVRPlayer::Grabbing()
{
	if (bIsGrabbed == false)
	{
		return;
	}

	// ���� ���� ������Ʈ
	throwDirection = rightHand->GetComponentLocation() - prevPos;

	// ȸ�� ���� ������Ʈ
	// ���ʹϿ� ����
	// angle1 = Q1, angle2 = Q2
	// angle1 + angle2 = Q1 * Q2
	// -angle = Q2.Inverse()
	// angle2 - angle1 = Q2 * Q1.Inverse()
	deltaRotation = rightHand->GetComponentQuat() * prevRot.Inverse();

	// ���� ��ġ ������Ʈ
	prevPos = rightHand->GetComponentLocation();

	// ���� ȸ�� ������Ʈ
	prevRot = rightHand->GetComponentQuat();
}