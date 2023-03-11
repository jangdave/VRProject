// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "VRPlayer.generated.h"

// 사용자의 입력에 따라 앞뒤좌우로 이동하고 싶다
// 필요속성 : 이동속도, 입력액션, 입력매핑컨텍스트
// 사용자가 텔레포트 버튼을 눌렀다가 떼면 텔레포트 되도록 한다
// 4. 텔레포트 버튼을 눌렀다가 떼서
// 3. 사용자가 그 지점을 가리키니까
// 2. 텔레포트 목적지가 필요하다
// 1. 텔레포트 이동 하고 싶다
// 연역적 접근

UCLASS()
class VRPROJECT_API AVRPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 필요속성 : 이동속도, 입력액션, 입력매핑컨텍스트
	// 이동속도
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float moveSpeed = 500.0f;

	// imc
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* IMC_VRInput;

	// ia
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Move;

	// 이동처리 함수
	void Move(const FInputActionValue& Values);

public: // 마우스 입력처리
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Mouse;

	// 회전처리 함수
	void Turn(const FInputActionValue& Values);

protected:
	UPROPERTY(VisibleAnywhere, Category = "VRCamera")
	class UCameraComponent* vrCamera;

	// 컨트롤러
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* leftHand;

	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* rightHand;

	// 사용할 손모델
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class USkeletalMeshComponent* leftHandMesh;

	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class USkeletalMeshComponent* rightHandMesh;

public: // teleport straight
	UPROPERTY(VisibleAnywhere, Category = "Teleport")
	class UNiagaraComponent* teleportCircle;

	// teleport 기능 활성화 여부
	bool bTeleporting = false;

	// teleport 버튼을 눌렀을때 함수
	void TeleportStart(const FInputActionValue& Values);

	// teleport 버튼을 뗐을때 함수
	void TeleportEnd(const FInputActionValue& Values);

	bool ResetTeleport();

	// 직선 텔레포트 처리하기
	void DrawTeleportStraight();

	// 텔레포트 선과 충돌체크 함수
	bool CheckHitTeleport(FVector lastPos, FVector& curPos);

	// 충돌 체크
	bool HitTest(FVector lastPos, FVector curPos, FHitResult& result);

	// 텔레포트 입력 액션
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Teleport;

	FVector teleportLocation;

private: // 곡선 텔레포트
	// 곡선 텔레포트 사용 여부
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	bool bTeleportCurve = true;

	// 던지는 힘
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float curvedPower = 1500.0f;

	// 중력
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float gravity = -5000.0f;

	// 시뮬레이션 시간
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float simulratedTime = 0.02f;

	// 곡선을 이루는 점의 개수
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	int32 lineSmooth = 40;

	// 점을 기억할 배열
	UPROPERTY()
	TArray<FVector> lines;

	void DrawTeleportCurve();

	// 사용할 나이아가라 컴포넌트(linetrace)
	UPROPERTY(VisibleAnywhere, Category = "Teleport")
	class UNiagaraComponent* teleportCurveComp;

private:
	// 워프 사용 여부
	UPROPERTY(EditAnywhere, Category = "Teleport", meta = (AllowPrivateAccess = true))
	bool bIsWarp = true;

	// 타이머
	UPROPERTY()
	FTimerHandle warpHandle;

	// 경과시간
	UPROPERTY()
	float curTime = 0;

	// 워프할때 걸리는 시간
	UPROPERTY(EditAnywhere, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float warpTime = 0.2f;

	// 워프 수행할 함수
	void DoWarp();

private: // 총쏘기
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = true))
	class UInputAction* IA_Fire;

	// 총쏘기 처리할 함수
	void FireInput(const FInputActionValue& Values);

	// 집게 손가락 표시할 모션 컨트롤러
	UPROPERTY(VisibleAnywhere, Category = "HandComp", meta = (AllowPrivateAccess = true))
	class UMotionControllerComponent* rightAim;

	// 크로스헤어
	UPROPERTY(EditAnywhere, Category = "CrossHair", meta = (AllowPrivateAccess = true))
	TSubclassOf<AActor> crossHairFactory;

	// 인스턴스 크로스헤어
	UPROPERTY()
	AActor* crossHair;

	// 크로스헤어 그리기
	void DrawCrossHair();

public: // 잡기 버튼을 누르면 물체를 잡고싶다
	// 필요 속성 : 입력 액션, 잡을 범위
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* IA_Grab;

	// 잡을 범위
	UPROPERTY(EditAnywhere, Category = "Grab")
	float GrabRange = 100.0f;

	// 잡은 물체 기억
	UPROPERTY()
	class UPrimitiveComponent* grabbedObject;

	// 잡을 녀석이 있는지 여부 기억할 변수
	bool bIsGrabbed = false;

	// 던지면 원하는 방향으로 날아가도록 하고 싶다
	// 던질 방향
	FVector throwDirection;

	// 던질 힘
	UPROPERTY(EditAnywhere, Category = "Grab")
	float throwPower = 1000.0f;

	// 직전 위치
	FVector prevPos;

	// 이전 회전
	FQuat prevRot;

	// 회전 방향
	FQuat deltaRotation;

	// 회전 빠르기
	UPROPERTY(EditAnywhere, Category = "Grab")
	float toquePower = 1.0f;

	// 잡기 시도 기능
	void TryGrab();

	// 놓기
	void UnTryGrab();

	// 잡고 있는중
	void Grabbing();

	UPROPERTY(EditDefaultsOnly, Category = "Haptic")
	class UHapticFeedbackEffect_Curve* HF_Fire;
};