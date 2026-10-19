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
#include "esminiLib.hpp"
#include "EsminiLog.h"
#include "Subsystems/GemstarSimulationSubsystem.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Movement/VehicleDataDispatcher.h"
#include "Interfaces/Movement/VRUDataUpdate.h"
#include "Interfaces/Weather/WeatherDataUpdate.h"
#include "Kismet/KismetSystemLibrary.h"

#include "EsminiManager.generated.h"



UENUM(BlueprintType)
enum class EEsminiMode : uint8
{
	Unset UMETA(DisplayName = "Configure was not called"),
	Configured UMETA(DisplayName = "Configure was called"),
	Running UMETA(DisplayName = "esmini::load was called"),
	Done UMETA(DisplayName = "esmini is done")
};

USTRUCT(BlueprintType)
struct FScenarioObjectState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int id;              // Automatically generated unique object id
	UPROPERTY(BlueprintReadOnly) int model_id;        // Id to control what 3D model to represent the vehicle - see carModelsFiles_[] in scenarioenginedll.cpp
	UPROPERTY(BlueprintReadOnly) int ctrl_type;       // 0: DefaultController 1: External. Further values see Controller::Type enum
	UPROPERTY(BlueprintReadOnly) float timestamp;     // Not used yet (idea is to use it to interpolate position for increased sync bewtween simulators)
	UPROPERTY(BlueprintReadOnly) float x;             // global x coordinate of position
	UPROPERTY(BlueprintReadOnly) float y;             // global y coordinate of position
	UPROPERTY(BlueprintReadOnly) float z;             // global z coordinate of position
	UPROPERTY(BlueprintReadOnly) float h;             // heading/yaw in global coordinate system
	UPROPERTY(BlueprintReadOnly) float p;             // pitch in global coordinate system
	UPROPERTY(BlueprintReadOnly) float r;             // roll in global coordinate system
	UPROPERTY(BlueprintReadOnly) int roadId;          // road ID
	UPROPERTY(BlueprintReadOnly) int junctionId;      // Junction ID (-1 if not in a junction)
	UPROPERTY(BlueprintReadOnly) float t;             // lateral position in road coordinate system
	UPROPERTY(BlueprintReadOnly) int laneId;          // lane ID
	UPROPERTY(BlueprintReadOnly) float laneOffset;    // lateral offset from lane center
	UPROPERTY(BlueprintReadOnly) float s;             // longitudinal position in road coordinate system
	UPROPERTY(BlueprintReadOnly) float speed;         // speed
	UPROPERTY(BlueprintReadOnly) float centerOffsetX; // x coordinate of bounding box center relative object reference point (local coordinate system)
	UPROPERTY(BlueprintReadOnly) float centerOffsetY; // y coordinate of bounding box center relative object reference point (local coordinate system)
	UPROPERTY(BlueprintReadOnly) float centerOffsetZ; // z coordinate of bounding box center relative object reference point (local coordinate system)
	UPROPERTY(BlueprintReadOnly) float width;         // width
	UPROPERTY(BlueprintReadOnly) float length;        // length
	UPROPERTY(BlueprintReadOnly) float height;        // height
	UPROPERTY(BlueprintReadOnly) int objectType;      // Main type according to entities.hpp / Object / Type
	UPROPERTY(BlueprintReadOnly) int objectCategory;  // Sub category within type, according to entities.hpp / Vehicle, Pedestrian, MiscObject / Category
	FScenarioObjectState() {
		id = 0;
		model_id = 0;
		ctrl_type = 0;
		timestamp = 0.0;
		x = 0.0;
		y = 0.0;
		z = 0.0;
		h = 0.0;
		p = 0.0;
		r = 0.0;
		roadId = 0;
		junctionId = 0;
		t = 0.0;
		laneId = 0;
		s = 0.0;
		laneOffset = 0.0;
		s = 0.0;
		speed = 0.0;
		centerOffsetX = 0.0;
		centerOffsetY = 0.0;
		centerOffsetZ = 0.0;
		width = 0.0;
		length = 0.0;
		height = 0.0;
		objectType = 0;
		objectCategory = 0;
	}


	FScenarioObjectState(SE_ScenarioObjectState& other) {
		id = other.id;
		model_id = other.model_id;
		ctrl_type = other.ctrl_type;
		timestamp = other.timestamp;
		x = other.x;
		y = other.y;
		z = other.z;
		h = other.h;
		p = other.p;
		r = other.r;
		roadId = other.roadId;
		junctionId = other.junctionId;
		t = other.t;
		laneId = other.laneId;
		s = other.s;
		laneOffset = other.laneOffset;
		s = other.s;
		speed = other.speed;
		centerOffsetX = other.centerOffsetX;
		centerOffsetY = other.centerOffsetY;
		centerOffsetZ = other.centerOffsetZ;
		width = other.width;
		length = other.length;
		height = other.height;
		objectType = other.objectType;
		objectCategory = other.objectCategory;
	}
};

UCLASS()
class UE_ESMINI_FEATURERUNTIME_API AEsminiManager : public AActor,
	public IVehicleDataDispatcher,
	public IVehicleDataUpdate,
	public IVRUDataUpdate,
	public IWeatherDataUpdate,
	public IGemstarManager
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEsminiManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RegisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver) override;

	void UnregisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver) override;

	virtual void VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void Configure(TMap<FString, FInnerConfig> Configurations, UGemstarSimulationSubsystem* Root);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "esmini")
	bool bUseBPConfiguration = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "esmini")
	FString Path = "simple-left-turn/scenario_scenario1_0.xosc";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "esmini")
	FString ExampleParameter1 = "simple-left-turn/scenario_scenario1_0.xosc";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "esmini")
	FString ExampleParameter2 = "cut-in/cut-in.xosc";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "esmini")
	EEsminiMode Mode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "esmini")
	TArray<FScenarioObjectState> ScenarioObjectsArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "esmini")
	TMap<FString, FTransform> transforms;

	UFUNCTION(BlueprintCallable, Category = "esmini")
	void LoadScenario();

	UPROPERTY(EditAnywhere, Category = "esmini")
	FTransform CoordinateSystemOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "esmini")
	bool bDebug = false;

private:
	FName PROVIDER_NAME{ "ue_esmini_feature" };

	TArray<FDataTarget> VehicleActorsToInform;
	TArray<FDataTarget> VRUActorsToInform;
	TArray<FDataTarget> WeatherEffectActorsToInform;

	FString GetOpenSCENARIORootDirectory();

	void SendObjectUpdates();
	void SendWeatherUpdate();


	FTransform TranslateToUnrealWorldPosition(const FScenarioObjectState& obj);
protected:
	UPROPERTY()
	TObjectPtr<UGemstarSimulationSubsystem> GemstarSimulationSubsystem;
	friend class AEsminiTester;
};
