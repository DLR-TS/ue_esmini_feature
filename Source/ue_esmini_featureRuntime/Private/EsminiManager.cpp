// Fill out your copyright notice in the Description page of Project Settings.


#include "EsminiManager.h"

FString GetAbsolutePathOfGameFeatureFile(const FString& RelativeFilePath, const FString& PluginName = TEXT("ue_esmini_feature"))
{
	//FString PluginName = TEXT("esmini");

	// Get the plugin directory
	FString PluginBaseDir = IPluginManager::Get().FindPlugin(PluginName)->GetBaseDir();

	// Combine with relative path inside the GameFeature
	FString FullPath = FPaths::Combine(PluginBaseDir, RelativeFilePath);

	// Convert to absolute path
	return FPaths::ConvertRelativePathToFull(FullPath);
}

// Sets default values
AEsminiManager::AEsminiManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Mode = EEsminiMode::Unset;
}

// Called when the game starts or when spawned
void AEsminiManager::BeginPlay()
{
	Super::BeginPlay();
	if (bUseBPConfiguration) {
		FString p = GetAbsolutePathOfGameFeatureFile(Path);
		UE_LOG(LogTemp, Log, TEXT("esmini: %s "), *p);
		Mode = EEsminiMode::Configured;
	}
}

void AEsminiManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SE_Close();
}

// Called every frame
void AEsminiManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ActorsToInform.Num() == 0) {
		GemstarRoot->ConfigureDataFlow();
	}

	ScenarioObjectsArray.Reset();
	transforms.Reset();
	if (Mode == EEsminiMode::Configured) {
		FString p = GetAbsolutePathOfGameFeatureFile(Path);
		UE_LOG(LogTemp, Log, TEXT("esmini: %s "), *p);
		LoadScenario(p);
		Mode = EEsminiMode::Running;
	}
	if (Mode != EEsminiMode::Running) {
		return;
	}

	SE_StepDT(DeltaTime);
	if (SE_GetQuitFlag() == 1) {
		UE_LOG(LogTemp, Log, TEXT("esmini done with scenario"));
		Mode = EEsminiMode::Done;

		if (GIsEditor)
		{
			// Running inside Unreal Editor
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("Exit"));
		}
		else
		{
			// Running as packaged game or standalone
			UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
		}

		return;
	}
	SE_ScenarioObjectState state;
	for (int j = 0; j < SE_GetNumberOfObjects(); j++)
	{
		SE_GetObjectState(SE_GetId(j), &state);
		int id = SE_GetId(j);
		FString objName(SE_GetObjectName(id));
		FString typeName(SE_GetObjectTypeName(id));
		ScenarioObjectsArray.Add(FScenarioObjectState(state));
		FTransform t = TranslateToUnrealWorldPosition(state);
		transforms.Add(objName, t);
		//UE_LOG(LogTemp, Warning, TEXT("player: %s -> %s"), *objName, *typeName);
		//UE_LOG(LogTemp, Warning, TEXT("time [%.2f] object[%d] id %d pos[%.2f, %.2f, %.2f] \n"), state.timestamp, j, SE_GetId(j), state.x, state.y, state.y);

		FVehicleDataUpdateData newMessage;
		newMessage.Name = FName(objName);
		newMessage.VehicleClass = FName(typeName);
		newMessage.Position = t;
		newMessage.accel = 0.0f;
		newMessage.speed = state.speed;
		newMessage.BoundingBox.Set(state.length, state.width, state.height);
		newMessage.Provider = PROVIDER_NAME;
		for (auto& i : ActorsToInform) {
			IVehicleDataUpdate::Execute_VehicleDataUpdate(i, newMessage);
		}
	}
}

void AEsminiManager::Configure(TMap<FString, FInnerConfig> Configurations, AGEMSTARRoot* Root) {
	GemstarRoot = Root;
	if (Configurations.Num() > 1) {
		UE_LOG(LogTemp, Error, TEXT("esmini: Got multiple configurations."));
		return;
	}
	for (const TPair<FString, FInnerConfig>& Pair : Configurations)
	{
		const FString* valuePath = Pair.Value.Entries.Find("Path");
		if (valuePath != nullptr) {
			Path = *valuePath;
		}
		const FString* valueCoordinateSystemOffset = Pair.Value.Entries.Find("CoordinateSystemOffset");
		if (valueCoordinateSystemOffset != nullptr) {
			CoordinateSystemOffset.InitFromString(*valueCoordinateSystemOffset);
		}
		const FString* valueDebug = Pair.Value.Entries.Find("Debug");
		if (valueDebug != nullptr) {
			bDebug = (valueDebug->Equals("true", ESearchCase::IgnoreCase));
		}
	}
	Mode = EEsminiMode::Configured;
}

void AEsminiManager::LoadScenario(FString oscFile) {
	int ret = SE_Init(TCHAR_TO_ANSI(*oscFile), 0, 0, 0, 0);
	UE_LOG(LogTemp, Warning, TEXT("esmini: %d "), ret);
	if (ret == 0) {
		Mode = EEsminiMode::Running;
	}
}

void AEsminiManager::SendTestMessage()
{
	FVehicleDataUpdateData newMessage;
	FVector position{ 1000, 500, 10 };
	newMessage.Position.SetLocation(position);
	for (auto& i : ActorsToInform) {
		IVehicleDataUpdate::Execute_VehicleDataUpdate(i, newMessage);
	}
}

FTransform AEsminiManager::TranslateToUnrealWorldPosition(const FScenarioObjectState& obj)
{
	//FTransform transform;
	//transform.SetLocation(CoordinateSystemOffset.InverseTransformPosition(FVector(obj.x * 100, -obj.y * 100, obj.z * 100)));
	//transform.SetRotation(CoordinateSystemOffset.InverseTransformRotation(FRotator(FMath::RadiansToDegrees(obj.p), (-FMath::RadiansToDegrees(obj.h)), FMath::RadiansToDegrees(obj.r)).Quaternion()));
	//return transform;
	FVector posUE(
		obj.x * 100,
		-obj.y * 100,
		obj.z * 100
	);

	float pitch = FMath::RadiansToDegrees(obj.p);
	float yaw = -FMath::RadiansToDegrees(obj.h);
	float roll = FMath::RadiansToDegrees(obj.r);

	//UE_LOG(LogTemp, Warning, TEXT("%f"), yaw);

	FTransform transform;
	transform.SetLocation(posUE);
	transform.SetRotation(FRotator(pitch, yaw, roll).Quaternion());
	return transform;
}

// eventOnNewVehicleData -> {Positions (Transform), Name (Name), BoundingBox(Vector(x,y,z), Type(Name)} 

void AEsminiManager::VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) {
	if (data.Provider.IsEqual(PROVIDER_NAME)) {
		//skip, use for testing
		UE_LOG(LogTemp, Warning, TEXT("esmini: should not receive own data %s "), *data.Name.ToString());
	}
	else {
		//todo
	}
}

bool AEsminiManager::RegisterDataflow_Implementation(AActor* consumer, FString& producer, GEMSTARInterfaceType interfaceName) {
	if (interfaceName == GEMSTARInterfaceType::TRAFFICUPDATE) {
		ActorsToInform.AddUnique(consumer);
		consumer->AddTickPrerequisiteActor(this);
		return true;
	}
	return false;
}

void AEsminiManager::RegisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>()) {
		ActorsToInform.AddUnique(vehicleDataReceiver);
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Tried to register an actor without UEsminiVehicleDataUpdate."));
	}
}

void AEsminiManager::UnregisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>()) {
		ActorsToInform.Remove(vehicleDataReceiver);
	}
}
