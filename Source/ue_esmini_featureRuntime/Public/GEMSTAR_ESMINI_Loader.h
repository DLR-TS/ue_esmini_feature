/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#pragma once

#include "EsminiLog.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GEMSTAR_ESMINI_Loader.generated.h"

class AEsminiManager;

UCLASS(ClassGroup = (GEMSTAR), meta = (BlueprintSpawnableComponent))
class UE_ESMINI_FEATURERUNTIME_API UGEMSTAR_ESMINI_Loader : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGEMSTAR_ESMINI_Loader();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<AEsminiManager> EsminiManager;
};
