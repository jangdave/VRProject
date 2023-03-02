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
	class UStaticMeshComponent* teleportCircle;

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
};