// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "VRPlayer.generated.h"

// ������� �Է¿� ���� �յ��¿�� �̵��ϰ� �ʹ�
// �ʿ�Ӽ� : �̵��ӵ�, �Է¾׼�, �Է¸������ؽ�Ʈ
// ����ڰ� �ڷ���Ʈ ��ư�� �����ٰ� ���� �ڷ���Ʈ �ǵ��� �Ѵ�
// 4. �ڷ���Ʈ ��ư�� �����ٰ� ����
// 3. ����ڰ� �� ������ ����Ű�ϱ�
// 2. �ڷ���Ʈ �������� �ʿ��ϴ�
// 1. �ڷ���Ʈ �̵� �ϰ� �ʹ�
// ������ ����

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

public: // teleport straight
	UPROPERTY(VisibleAnywhere, Category = "Teleport")
	class UStaticMeshComponent* teleportCircle;

	// teleport ��� Ȱ��ȭ ����
	bool bTeleporting = false;

	// teleport ��ư�� �������� �Լ�
	void TeleportStart(const FInputActionValue& Values);

	// teleport ��ư�� ������ �Լ�
	void TeleportEnd(const FInputActionValue& Values);

	bool ResetTeleport();

	// ���� �ڷ���Ʈ ó���ϱ�
	void DrawTeleportStraight();

	// �ڷ���Ʈ ���� �浹üũ �Լ�
	bool CheckHitTeleport(FVector lastPos, FVector& curPos);

	// �浹 üũ
	bool HitTest(FVector lastPos, FVector curPos, FHitResult& result);

	// �ڷ���Ʈ �Է� �׼�
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Teleport;

	FVector teleportLocation;
};