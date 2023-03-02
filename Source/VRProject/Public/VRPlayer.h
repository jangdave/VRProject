// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "VRPlayer.generated.h"

// ������� �Է¿� ���� �յ��¿�� �̵��ϰ� �ʹ�
// �ʿ�Ӽ� : �̵��ӵ�, �Է¾׼�, �Է¸������ؽ�Ʈ

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

	// �ʿ�Ӽ� : �̵��ӵ�, �Է¾׼�, �Է¸������ؽ�Ʈ
	// �̵��ӵ�
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float moveSpeed = 500.0f;

	// imc
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* IMC_VRInput;

	// ia
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Move;

	// �̵�ó�� �Լ�
	void Move(const FInputActionValue& Values);

public: // ���콺 �Է�ó��
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Mouse;

	// ȸ��ó�� �Լ�
	void Turn(const FInputActionValue& Values);

protected:
	UPROPERTY(VisibleAnywhere, Category = "VRCamera")
	class UCameraComponent* vrCamera;

	// ��Ʈ�ѷ�
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* leftHand;

	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* rightHand;

	// ����� �ո�
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class USkeletalMeshComponent* leftHandMesh;

	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class USkeletalMeshComponent* rightHandMesh;
};
