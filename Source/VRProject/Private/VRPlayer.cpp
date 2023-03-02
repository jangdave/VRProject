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

// Sets default values
AVRPlayer::AVRPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	vrCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
	vrCamera->SetupAttachment(RootComponent);
	vrCamera->bUsePawnControlRotation = true;

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
	teleportCircle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("teleportCircle"));
	teleportCircle->SetupAttachment(RootComponent);
	teleportCircle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
		}
	}

	ResetTeleport();
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
	}

	// �ڷ���Ʈ Ȯ�� ó��
	if(bTeleporting)
	{
		DrawTeleportStraight();
	}
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
	}
}

void AVRPlayer::Move(const FInputActionValue& Values)
{
	UE_LOG(LogTemp, Warning, TEXT("move !!!"));
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
	// �ڷ���Ʈ ��ġ�� �̵��ϰ� �ʹ�
	SetActorLocation(teleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

bool AVRPlayer::ResetTeleport()
{
	// �ڷ���Ʈ ��Ŭ�� ���϶��� �ڷ���Ʈ ����
	bool bCanTeleport = teleportCircle->GetVisibleFlag();

	// ��Ŭ �Ⱥ��̰� ó��
	teleportCircle->SetVisibility(false);
	teleportLocation = teleportCircle->GetComponentLocation();
	bTeleporting = false;

	return bCanTeleport;
}

void AVRPlayer::DrawTeleportStraight()
{
	// ������ �׸��� �ʹ�.
	// �ʿ����� : ������, ������
	FVector startPos = rightHand->GetComponentLocation();
	FVector endPos = startPos + rightHand->GetForwardVector() * 1000.0f;

	// �� �� ���̿� �浹�� �Ǵ��� üũ
	CheckHitTeleport(startPos, endPos);
	
	DrawDebugLine(GetWorld(), startPos, endPos, FColor::Red, false, -1, 0, 1);
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
