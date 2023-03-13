// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuWidget.h"
#include "Kismet/KismetSystemLibrary.h"

void UMenuWidget::QuitVRGame()
{
	auto PC = GetWorld()->GetFirstPlayerController();

	if (PC)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, true);
	}
}
