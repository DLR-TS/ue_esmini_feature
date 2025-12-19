// Fill out your copyright notice in the Description page of Project Settings.


#include "GEMSTAR_ESMINI_Loader.h"

// Sets default values for this component's properties
UGEMSTAR_ESMINI_Loader::UGEMSTAR_ESMINI_Loader()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UGEMSTAR_ESMINI_Loader::BeginPlay()
{
	Super::BeginPlay();
	AActor* OwnerActor = GetOwner();
	AGEMSTARRoot* GemstarRoot = Cast<AGEMSTARRoot>(OwnerActor);
	if (GemstarRoot)
	{
		if (GemstarRoot->UseESMINI)
		{
			FTransform SpawnTransform;

			FActorSpawnParameters SpawnParams;
			AEsminiManager* SpawnedEsminiManager = GetWorld()->SpawnActor<AEsminiManager>(AEsminiManager::StaticClass(), SpawnTransform, SpawnParams);

			TMap<FString, FInnerConfig> config = GemstarRoot->RegisterManagerAndGetConfiguration(SpawnedEsminiManager);
			SpawnedEsminiManager->Configure(config, GemstarRoot);
			UE_LOG(LogTemp, Log, TEXT("Registered and configured Esmini"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Error: Cast of GEMSTARRoot was not successful"));
	}
}


// Called every frame
void UGEMSTAR_ESMINI_Loader::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

