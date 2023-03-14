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

	// 스켈레탈 컴포넌트 만들기
	leftHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("leftHandMesh"));
	leftHandMesh->SetupAttachment(leftHand);
	rightHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("rightHandMesh"));
	rightHandMesh->SetupAttachment(rightHand);

	// 스켈레탈 메시 로드해서 할당
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

	// 집게손가락
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

	// enhanced input 사용처리
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

	// hmd가 연결 되어 있지 않다면
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() == false)
	{
		// 손을 테스트 할 수 있는 위치로 이동시키자
		rightHand->SetRelativeLocation(FVector(20.0f, 20.0f, 0.0f));
		rightAim->SetRelativeLocation(FVector(20.0f, 20.0f, 0.0f));

		// 카메라의 use pawn control rotation 활성화
		vrCamera->bUsePawnControlRotation = true;
	}
	// 만약에 hmd가 연결되어 있다면
	else
	{
		// -> 기본 트렉킹 offset 설정
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye);
	}
}

// Called every frame
void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// HMD 가 연결되어 있지 않으면
	if(UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() == false)
	{
		// -> 손이 카메라 방향과 일치하도록 한다
		rightHand->SetRelativeRotation(vrCamera->GetRelativeRotation());
		rightAim->SetRelativeRotation(vrCamera->GetRelativeRotation());
	}

	// corsshair
	DrawCrossHair();

	// 텔레포트 확인 처리
	if(bTeleporting)
	{
		// 만약 직선을 그린다면
		if(bTeleportCurve == false)
		{
			DrawTeleportStraight();
		}
		// 그렇지 않으면
		else
		{
			// 곡선 그리기
			DrawTeleportCurve();
		}

		// 나이아가라 이용해서 선그리기
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
		// 잡기
		inputSystem->BindAction(IA_Grab, ETriggerEvent::Started, this, &AVRPlayer::TryGrab);
		inputSystem->BindAction(IA_Grab, ETriggerEvent::Completed, this, &AVRPlayer::UnTryGrab);
	}
}

void AVRPlayer::Move(const FInputActionValue& Values)
{
	// 1. 사용자의 입력에 따라
	FVector2D Axis = Values.Get<FVector2D>();

	AddMovementInput(GetActorForwardVector(), Axis.X);

	AddMovementInput(GetActorRightVector(), Axis.Y);

	// 2. 앞뒤 좌우라는 방향이 필요
	//FVector dir = FVector(Axis.X, Axis.Y, 0);

	// 3. 이동하고 싶다
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

// 텔레포트 기능 활성화
void AVRPlayer::TeleportStart(const FInputActionValue& Values)
{
	// 눌렀을때 사용자가 어디를 가르키는지 주시하고 싶다
	bTeleporting = true;

	// 라인이 보이도록 활성화
	teleportCurveComp->SetVisibility(true);
}

void AVRPlayer::TeleportEnd(const FInputActionValue& Values)
{
	// 텔레포트 기능 리셋
	// 만약 텔레포트가 불가능하다면
	// 다음 처리를 하지 않는다
	if(ResetTeleport() == false)
	{
		return;
	}

	// 워프 사용시 워프 처리하고
	if (bIsWarp)
	{
		DoWarp();
	}
	// 그렇지 않으면
	else
	{
		// 텔레포트 위치로 이동하고 싶다
		SetActorLocation(teleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
}

bool AVRPlayer::ResetTeleport()
{
	// 텔레포트 서클이 보일때만 텔레포트 가능
	bool bCanTeleport = teleportCircle->GetVisibleFlag();

	// 서클 안보이게 처리
	teleportCircle->SetVisibility(false);
	teleportLocation = teleportCircle->GetComponentLocation();
	bTeleporting = false;
	teleportCurveComp->SetVisibility(false);

	return bCanTeleport;
}

void AVRPlayer::DrawTeleportStraight()
{
	lines.RemoveAt(0, lines.Num());
	// 직선을 그리고 싶다.
	// 필요정보 : 시작점, 종료점
	FVector startPos = rightAim->GetComponentLocation();
	FVector endPos = startPos + rightAim->GetForwardVector() * 1000.0f;

	// 두 점 사이에 충돌이 되는지 체크
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
		// 마지막 점을 최종 점으로 수정
		curPos = result.Location;
		// 써클 활성화
		teleportCircle->SetVisibility(true);
		// 텔레포트 써클을 위치
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

// 주어진 속도로 투사체를 날려보내고 투사체의 지나간 점을 기록
void AVRPlayer::DrawTeleportCurve()
{
	// lines 초기화
	lines.RemoveAt(0, lines.Num());
	// 4. 시작점, 방향, 힘 으로 투사체를 던진다
	FVector pos = rightAim->GetComponentLocation();
	FVector dir = rightAim->GetForwardVector() * curvedPower;

	// 시작점을 가장 먼저 기록
	lines.Add(pos);

	for (int i = 0; i < lineSmooth; i++)
	{
		// 이전 점 기억
		FVector lastPos = pos;
		// 3. 투사체가 이동했으니까(반복적으로)
		// v = v0 +at
		dir += FVector::UpVector * gravity * simulratedTime;
		// p = p0 +vt
		pos += dir * simulratedTime;
		// 2. 투사체의 위치에서
		// -> 점과 점 사이에 물체가 가로막고 있다면
		if (CheckHitTeleport(lastPos, pos))
		{
			//     -> 그 점을 마지막 점으로 하자
			lines.Add(pos);
			break;
		}
		// 1. 점을 기록하자
		lines.Add(pos);
	}

	//for(int i = 0; i<lines.Num() - 1; i++)
	//{
		//곡선 그리기
	//	DrawDebugLine(GetWorld(), lines[i], lines[i + 1], FColor::Red, false, -1, 0, 1);
	//}
}

void AVRPlayer::DoWarp()
{
	// 워프 기능이 활성화 되어 있을때
	if (bIsWarp != true)
	{
		return;
	}
	// 워프 처리 하고 싶다
	// ->일정 시간동안 빠르게 이동
	// 경과 시간을 초기화
	curTime = 0;
	// 컬리젼 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 3. 시간이 흘러야한다
	// 2. 일정시간동안
	// [캡쳐] ( ) -> { body }
	GetWorld()->GetTimerManager().SetTimer(warpHandle, FTimerDelegate::CreateLambda(
		[this]()->void
		{
			// body
			// 일정 시간안에 목적지에 도착
			// 3. 시간이 흐른다
			curTime += GetWorld()->DeltaTimeSeconds;
			// 현재
			FVector curPos = GetActorLocation();
			// 도착
			FVector endPos = teleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			// 2. 이동한다
			curPos = FMath::Lerp<FVector>(curPos, endPos, curTime / warpTime);
			// 1. 목적지에 도착한다
			SetActorLocation(curPos);

			// 시간이 다 흘렀다면
			if (curTime >= warpTime)
			{
				// ->그 위치로 할당한다
				SetActorLocation(endPos);
				// ->타이머 종료(워프 종료)
				GetWorld()->GetTimerManager().ClearTimer(warpHandle);

				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
			// 거리가 거의 가까워 지면 그 위치로 할당해주기
			// _________________________루트 계산은 느리다___________________________
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
	// UI로 이벤트 전달
	if (widgetInteractionComp)
	{
		widgetInteractionComp->PressPointerKey(FKey(FName("LeftMouseButton")));
		//widgetInteractionComp->PressPointerKey(FKey(FName("OculusTouch(R)Trigger")));
	}

	// 진동처리
	auto PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->PlayHapticEffect(HF_Fire, EControllerHand::Right);
	}

	// linetrace 이용해서 총을 쏘고 싶다
	// 시작점
	FVector startPos = rightAim->GetComponentLocation();
	// 종료점
	FVector endPos = startPos + rightAim->GetForwardVector() * 10000;
	// 총쏘기(linetrace 동작)
	FHitResult hitInfo;
	bool bHit = HitTest(startPos, endPos, hitInfo);
	// 만약 부딪힌 것이 있다면 날린다
	if (bHit)
	{
		auto hitComp = hitInfo.GetComponent();
		if (hitComp && hitComp->IsSimulatingPhysics())
		{
			// 날려보내자
			hitComp->AddForceAtLocation(hitComp->GetMass() * (endPos - startPos).GetSafeNormal() * 100000.0f, hitInfo.Location);
		}
	}
}

// 거리에 따라서 크로스헤어 크기가 같게 보이도록 하자
void AVRPlayer::DrawCrossHair()
{
	// 시작점
	FVector startPos = rightAim->GetComponentLocation();
	// 끝점
	FVector endPos = startPos + rightAim->GetForwardVector() * 10000.0f;
	// 충돌 정보를 저장
	FHitResult hitInfo;
	// 충돌 체크
	bool bHit = HitTest(startPos, endPos, hitInfo);

	float distance = 0;

	if (bHit)
	{
		// -> 충돌이 발생하면 충돌한 지점에 크로스헤어 표시
		crossHair->SetActorLocation(hitInfo.Location);
		distance = hitInfo.Distance;
	}
	// 그렇지 않으면
	else
	{
		// -> 그냥 끝점에 크로스헤어 표시
		crossHair->SetActorLocation(endPos);
		distance = (endPos - startPos).Size();
	}

	crossHair->SetActorScale3D(FVector(FMath::Max<float>(1, distance)));

	// 빌보딩
	// ->크로스헤어가 카메라를 바라보도록 처리
	FVector direction = crossHair->GetActorLocation() - vrCamera->GetComponentLocation();
	crossHair->SetActorRotation(direction.Rotation());
}

// 물체를 잡고 싶다
void AVRPlayer::TryGrab()
{
	// 중심점
	FVector center = rightHand->GetComponentLocation();

	// 충돌체크(구 충돌)
	// 충돌한 물체를 기록할 배열
	TArray<FOverlapResult> hitObjs;
	// 충돌 질의 작성
	FCollisionQueryParams param;
	param.AddIgnoredActor(this);
	param.AddIgnoredComponent(rightHand);

	bool bHit = GetWorld()->OverlapMultiByChannel(hitObjs, center, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(GrabRange), param);

	// 충돌 하지 않았다면 아무 처리 하지 않는다
	if (bHit == false)
	{
		return;
	}
	// -> 가장 가까운 물체 잡도록 한다 (검출과정)
	// 가장 가까운 물체 인덱스
	int closest = 0;

	for(int i = 0; i<hitObjs.Num(); i++)
	{
		// 1. 물리 기능이 활성화 되어 있는 녀석만 판단
		// -> 만약 부딪힌 컴포넌트가 물리기능이 비활성화 되어 있다면
		if (hitObjs[i].GetComponent()->IsSimulatingPhysics() == false)
		{
			// 검출하고 싶지 않다
			continue;
		}
		// 잡았다
		bIsGrabbed = true;
		// 2. 손과 가장 가까운 녀석과 이번에 검출할 녀석과 더 가까운 녀석이 있다면
		// -> 필요속성 : 현재 가장 가까운 녀석과 손과의 거리
		float closestDist = FVector::Dist(hitObjs[closest].GetActor()->GetActorLocation(), center);
		// -> 필요속성 : 이번에 검출할 녀석과 손과의 거리
		float nextDist = FVector::Dist(hitObjs[i].GetActor()->GetActorLocation(), center);
		// 3. 만약 이번에가 현재꺼 보다 더 가깝다면
		if (nextDist < closestDist)
		{
			// -> 가장 가까운 녀석으로 변셩하기
			closest = i;
		}
	}

	// 만약 잡았다면
	if (bIsGrabbed)
	{
		grabbedObject = hitObjs[closest].GetComponent();
		// -> 물체 물리기능 비활성화
		grabbedObject->SetSimulatePhysics(false);
		grabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// -> 손에 붙여주자
		grabbedObject->AttachToComponent(rightHand, FAttachmentTransformRules::KeepWorldTransform);

		prevPos = rightHand->GetComponentLocation();
	}
}

// 잡은 녀석이 있으면 놓고 싶다
void AVRPlayer::UnTryGrab()
{
	if (bIsGrabbed == false)
	{
		return;
	}

	// 1. 잡지 않은 상태로 전화
	bIsGrabbed = false;
	// 2. 손에서 떼어내기
	grabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	// 3. 물리기능 활성화
	grabbedObject->SetSimulatePhysics(true);
	// 4. 충돌기능 활성화
	grabbedObject->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 던지기
	grabbedObject->AddForce(throwDirection * throwPower * grabbedObject->GetMass());

	// 회전시키기
	// 각속도 = (1 / dt) * dTheta(특정 축 기준 변위 각도 axis, angle)
	float angle;
	FVector axis;
	deltaRotation.ToAxisAndAngle(axis, angle);
	float dt = GetWorld()->GetDeltaSeconds();
	FVector angularVelocity = (1.0f / dt) * angle * axis;
	grabbedObject->SetPhysicsAngularVelocityInRadians(angularVelocity * toquePower, true);

	grabbedObject = nullptr;
}

// 던질 정보를 업데이트 하기 위한 기능
void AVRPlayer::Grabbing()
{
	if (bIsGrabbed == false)
	{
		return;
	}

	// 던질 방향 업데이트
	throwDirection = rightHand->GetComponentLocation() - prevPos;

	// 회전 방향 업데이트
	// 쿼터니온 공식
	// angle1 = Q1, angle2 = Q2
	// angle1 + angle2 = Q1 * Q2
	// -angle = Q2.Inverse()
	// angle2 - angle1 = Q2 * Q1.Inverse()
	deltaRotation = rightHand->GetComponentQuat() * prevRot.Inverse();

	// 이전 위치 업데이트
	prevPos = rightHand->GetComponentLocation();

	// 이전 회전 업데이트
	prevRot = rightHand->GetComponentQuat();
}