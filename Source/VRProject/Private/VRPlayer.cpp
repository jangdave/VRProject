// Fill out your copyright notice in the Description page of Project Settings.


#include "VRPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "VRGameModeBase.h"
#include "EnhancedInputComponent.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"

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
		}
	}
}

// Called every frame
void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	}
}

void AVRPlayer::Move(const FInputActionValue& Values)
{
	UE_LOG(LogTemp, Warning, TEXT("move !!!"));
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
