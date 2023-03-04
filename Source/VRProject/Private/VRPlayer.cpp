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

	// teleport
	teleportCircle = CreateDefaultSubobject<UNiagaraComponent>(TEXT("teleportCircle"));
	teleportCircle->SetupAttachment(RootComponent);
	teleportCircle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	teleportCurveComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("teleportCurveComp"));
	teleportCurveComp->SetupAttachment(RootComponent);
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

	ResetTeleport();
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
	}

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
	// 텔레포트 위치로 이동하고 싶다
	SetActorLocation(teleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
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
	FVector startPos = rightHand->GetComponentLocation();
	FVector endPos = startPos + rightHand->GetForwardVector() * 1000.0f;

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
	FVector pos = rightHand->GetComponentLocation();
	FVector dir = rightHand->GetForwardVector() * curvedPower;

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
