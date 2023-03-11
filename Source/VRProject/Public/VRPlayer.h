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
	class UNiagaraComponent* teleportCircle;

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

private: // � �ڷ���Ʈ
	// � �ڷ���Ʈ ��� ����
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	bool bTeleportCurve = true;

	// ������ ��
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float curvedPower = 1500.0f;

	// �߷�
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float gravity = -5000.0f;

	// �ùķ��̼� �ð�
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float simulratedTime = 0.02f;

	// ��� �̷�� ���� ����
	UPROPERTY(EditDefaultsOnly, Category = "Teleport", meta = (AllowPrivateAccess = true))
	int32 lineSmooth = 40;

	// ���� ����� �迭
	UPROPERTY()
	TArray<FVector> lines;

	void DrawTeleportCurve();

	// ����� ���̾ư��� ������Ʈ(linetrace)
	UPROPERTY(VisibleAnywhere, Category = "Teleport")
	class UNiagaraComponent* teleportCurveComp;

private:
	// ���� ��� ����
	UPROPERTY(EditAnywhere, Category = "Teleport", meta = (AllowPrivateAccess = true))
	bool bIsWarp = true;

	// Ÿ�̸�
	UPROPERTY()
	FTimerHandle warpHandle;

	// ����ð�
	UPROPERTY()
	float curTime = 0;

	// �����Ҷ� �ɸ��� �ð�
	UPROPERTY(EditAnywhere, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float warpTime = 0.2f;

	// ���� ������ �Լ�
	void DoWarp();

private: // �ѽ��
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = true))
	class UInputAction* IA_Fire;

	// �ѽ�� ó���� �Լ�
	void FireInput(const FInputActionValue& Values);

	// ���� �հ��� ǥ���� ��� ��Ʈ�ѷ�
	UPROPERTY(VisibleAnywhere, Category = "HandComp", meta = (AllowPrivateAccess = true))
	class UMotionControllerComponent* rightAim;

	// ũ�ν����
	UPROPERTY(EditAnywhere, Category = "CrossHair", meta = (AllowPrivateAccess = true))
	TSubclassOf<AActor> crossHairFactory;

	// �ν��Ͻ� ũ�ν����
	UPROPERTY()
	AActor* crossHair;

	// ũ�ν���� �׸���
	void DrawCrossHair();

public: // ��� ��ư�� ������ ��ü�� ���ʹ�
	// �ʿ� �Ӽ� : �Է� �׼�, ���� ����
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* IA_Grab;

	// ���� ����
	UPROPERTY(EditAnywhere, Category = "Grab")
	float GrabRange = 100.0f;

	// ���� ��ü ���
	UPROPERTY()
	class UPrimitiveComponent* grabbedObject;

	// ���� �༮�� �ִ��� ���� ����� ����
	bool bIsGrabbed = false;

	// ������ ���ϴ� �������� ���ư����� �ϰ� �ʹ�
	// ���� ����
	FVector throwDirection;

	// ���� ��
	UPROPERTY(EditAnywhere, Category = "Grab")
	float throwPower = 1000.0f;

	// ���� ��ġ
	FVector prevPos;

	// ���� ȸ��
	FQuat prevRot;

	// ȸ�� ����
	FQuat deltaRotation;

	// ȸ�� ������
	UPROPERTY(EditAnywhere, Category = "Grab")
	float toquePower = 1.0f;

	// ��� �õ� ���
	void TryGrab();

	// ����
	void UnTryGrab();

	// ��� �ִ���
	void Grabbing();

	UPROPERTY(EditDefaultsOnly, Category = "Haptic")
	class UHapticFeedbackEffect_Curve* HF_Fire;
};