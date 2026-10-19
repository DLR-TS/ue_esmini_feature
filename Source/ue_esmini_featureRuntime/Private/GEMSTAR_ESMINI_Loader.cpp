/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */


#include "GEMSTAR_ESMINI_Loader.h"
#include "EsminiLog.h"
#include "EsminiManager.h"
#include "Subsystems/GemstarSimulationSubsystem.h"

UGEMSTAR_ESMINI_Loader::UGEMSTAR_ESMINI_Loader()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGEMSTAR_ESMINI_Loader::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	if (World == nullptr) {
		return;
	}

	UGemstarSimulationSubsystem* GemstarSimulationSubsystem = UGemstarSimulationSubsystem::Get(GetWorld());
	
	if (GemstarSimulationSubsystem == nullptr) {
		UE_LOG(Esmini, Error, TEXT("ue_esmini_feature: simulation subsystem unavailable"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();

	EsminiManager = World->SpawnActor<AEsminiManager>(
		AEsminiManager::StaticClass(), FTransform::Identity, SpawnParams);

	if (!IsValid(EsminiManager)) {
		UE_LOG(Esmini, Error, TEXT("ue_esmini_feature: could not spawn AEsminiManager"));
		return;
	}

	const TMap<FString, FInnerConfig> config = GemstarSimulationSubsystem->RegisterManagerAndGetConfiguration(
			EsminiManager, "ue_esmini_feature");
	EsminiManager->Configure(config, GemstarSimulationSubsystem);
	
	UE_LOG(Esmini, Log, TEXT("Registered and configured Esmini"));
}


void UGEMSTAR_ESMINI_Loader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(EsminiManager))
	{
		EsminiManager->Destroy();
		EsminiManager = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}