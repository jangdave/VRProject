// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "VRPlayer.generated.h"

// 사용자의 입력에 따라 앞뒤좌우로 이동하고 싶다
// 필요속성 : 이동속도, 입력액션, 입력매핑컨텍스트

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
};
