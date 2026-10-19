/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "EsminiManager.h"
#include "Utils/StandaloneWeatherParser.h"
#include "Misc/DateTime.h"
#include "XmlFile.h"

 // Sets default values
AEsminiManager::AEsminiManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Mode = EEsminiMode::Unset;

	auto Wire = [this](TArray<FDataTarget>* List)
		{
			return [this, List](FDataTarget& Target, const FString& Producer)
				{
					// Only requests addressed to this producer
					if (!Producer.Equals(PROVIDER_NAME.ToString(), ESearchCase::IgnoreCase)) { return false; }
					List->AddUnique(Target);
					Target.Actor->AddTickPrerequisiteActor(this);
					return true;
				};
		};
	auto Unwire = [this](TArray<FDataTarget>* List)
		{
			return [this, List](FDataTarget& Target, const FString&)
				{
					const int32 Removed = List->Remove(Target);
					if (Removed > 0 && Target.Actor)
					{
						Target.Actor->RemoveTickPrerequisiteActor(this);
					}
					return Removed > 0;
				};
		};

	AcceptChannel(GemstarInterfaceType::TRAFFICUPDATE, Wire(&VehicleActorsToInform), Unwire(&VehicleActorsToInform));
	AcceptChannel(GemstarInterfaceType::VRUUPDATE, Wire(&VRUActorsToInform), Unwire(&VRUActorsToInform));
	AcceptChannel(GemstarInterfaceType::WEATHERDATA, Wire(&WeatherEffectActorsToInform), Unwire(&WeatherEffectActorsToInform));
}

// Called when the game starts or when spawned
void AEsminiManager::BeginPlay()
{
	Super::BeginPlay();
	if (bUseBPConfiguration) {
		UE_LOG(Esmini, Log, TEXT("Start with BPConfiguration"));
		Mode = EEsminiMode::Configured;
	}
}

void AEsminiManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(GemstarSimulationSubsystem)) {
		GemstarSimulationSubsystem->UnregisterManager(this);
		GemstarSimulationSubsystem = nullptr;
	}

	for (FDataTarget DataTarget : VehicleActorsToInform) {
		AActor* Consumer = DataTarget.Actor;

		if (IsValid(Consumer)) {
			Consumer->RemoveTickPrerequisiteActor(this);
		}
	}
	VehicleActorsToInform.Reset();

	for (FDataTarget DataTarget : VRUActorsToInform) {
		AActor* Consumer = DataTarget.Actor;

		if (IsValid(Consumer)) {
			Consumer->RemoveTickPrerequisiteActor(this);
		}
	}
	VRUActorsToInform.Reset();

	for (FDataTarget DataTarget : WeatherEffectActorsToInform) {
		AActor* Consumer = DataTarget.Actor;

		if (IsValid(Consumer)) {
			Consumer->RemoveTickPrerequisiteActor(this);
		}
	}
	WeatherEffectActorsToInform.Reset();

	SE_Close();
	Mode = EEsminiMode::Unset;

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AEsminiManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (Mode) {
	case EEsminiMode::Unset:
		break;
	case EEsminiMode::Configured:
		LoadScenario();
		break;
	case EEsminiMode::Running:
		SE_StepDT(DeltaTime);
		if (SE_GetQuitFlag() == 1) {
			UE_LOG(Esmini, Log, TEXT("Done with scenario %s."), *Path);
			Mode = EEsminiMode::Done;
			break;
		}
		SendObjectUpdates();
		break;
	case EEsminiMode::Done:
		if (IsValid(GemstarSimulationSubsystem) && GemstarSimulationSubsystem->ReportScenarioFinished(this)) {
			Mode = EEsminiMode::Unset;
			return;
		}
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
		break;
	default:
		break;
	}
}


void AEsminiManager::Configure(TMap<FString, FInnerConfig> Configurations, UGemstarSimulationSubsystem* SimulationSubsystem) {
	GemstarSimulationSubsystem = SimulationSubsystem;

	if (Configurations.Num() > 1) {
		UE_LOG(Esmini, Error, TEXT("Got multiple configurations."));
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
		else {
			CoordinateSystemOffset.SetLocation(FVector3d());
			CoordinateSystemOffset.SetRotation(FQuat::Identity);
			//ignore scale
		}
		const FString* valueDebug = Pair.Value.Entries.Find("Debug");
		if (valueDebug != nullptr) {
			bDebug = (valueDebug->Equals("true", ESearchCase::IgnoreCase));
		}
	}
	Mode = EEsminiMode::Configured;
}

void AEsminiManager::LoadScenario() {
	//When the path is not a full path than make it absolute.
	if (!FPaths::FileExists(Path)) {
		UE_LOG(Esmini, Display, TEXT("File %s not exists. Try to build full path..."), *Path);
		FString OscRootDir = GetOpenSCENARIORootDirectory();
		UE_LOG(Esmini, Display, TEXT("OscRootDir in LoadScenario is '%s'"), *OscRootDir);
		FString FullPath = FPaths::Combine(OscRootDir, Path);
		if (!FPaths::FileExists(FullPath)) {
			UE_LOG(Esmini, Error, TEXT("File %s not exists."), *FullPath);
			Mode = EEsminiMode::Done;
			return;
		}
		else {
			UE_LOG(Esmini, Log, TEXT("Found ScenarioFile: '%s'"), *FullPath);
			Path = FullPath;
		}
	}

	const FString ScenarioDirectory = FPaths::GetPath(Path);
	const FString EsminiLogFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectLogDir(), TEXT("esmini.log")));

	SE_ClearPaths();
	const int ScenarioPathResult = SE_AddPath(TCHAR_TO_UTF8(*ScenarioDirectory));
	const int VehiclePathResult = SE_AddPath(TCHAR_TO_UTF8(*FPaths::Combine(ScenarioDirectory, TEXT("Vehicles"))));
	const int PedestrianPathResult = SE_AddPath(TCHAR_TO_UTF8(*FPaths::Combine(ScenarioDirectory, TEXT("Pedestrians"))));
	SE_SetLogFilePath(TCHAR_TO_UTF8(*EsminiLogFile));

	UE_LOG(Esmini, Log, TEXT("Scenario dir=%s addPath=%d vehicles=%d pedestrians=%d log=%s"),
		*ScenarioDirectory,
		ScenarioPathResult,
		VehiclePathResult,
		PedestrianPathResult,
		*EsminiLogFile);

	int Ret = SE_Init(TCHAR_TO_UTF8(*Path), 0, 0, 0, 0);
	if (Ret == 0) {
		Mode = EEsminiMode::Running;
		UE_LOG(Esmini, Log, TEXT("Initialization completed."));
		SendWeatherUpdate();
	}
	else {
		UE_LOG(Esmini, Error, TEXT("esmini is not initialized, please look into the log file %s."), *EsminiLogFile);
		Mode = EEsminiMode::Done;
	}

}

void AEsminiManager::SendObjectUpdates() {
	ScenarioObjectsArray.Reset();
	transforms.Reset();
	int n = SE_GetNumberOfObjects();

	for (int i = 0; i < n; ++i)
	{
		int id = SE_GetId(i);
		FString objName(SE_GetObjectName(id));
		FString typeName(SE_GetObjectTypeName(id));

		SE_ScenarioObjectState state{};

		if (SE_GetObjectState(id, &state) == 0)
		{
			ScenarioObjectsArray.Add(FScenarioObjectState(state));
			FTransform t = TranslateToUnrealWorldPosition(state);
			transforms.Add(objName, t);
			switch (state.objectType)
			{
			case 1: // VEHICLE
			{
				FVehicleDataUpdateData VehicleData;
				VehicleData.Name = FName(objName);
				VehicleData.VehicleClass = FName(typeName);
				VehicleData.Position = t;
				VehicleData.accel = 0.0f;
				VehicleData.speed = state.speed;
				VehicleData.BoundingBox.Set(state.length, state.width, state.height);
				VehicleData.Provider = PROVIDER_NAME;
				for (auto& recipient : VehicleActorsToInform) {
					IVehicleDataUpdate::Execute_VehicleDataUpdate(recipient.Actor, VehicleData);
				}
				break;
			}

			case 2: // PEDESTRIAN
			{
				FVRUDataUpdateData VRUData;
				VRUData.Name = FName(objName);
				VRUData.Position = t;
				VRUData.Provider = PROVIDER_NAME;
				VRUData.speed = state.speed;
				VRUData.BoundingBox.Set(state.length, state.width, state.height);
				for (auto& recipient : VRUActorsToInform) {
					IVRUDataUpdate::Execute_VRUDataUpdate(recipient.Actor, VRUData);
				}
				break;
			}

			case 3: // MISC_OBJECT
			{
				UE_LOG(Esmini, Display, TEXT("Unhandled MISC_OBJECT %s"), *objName);
				break;
			}

			default:
			{
				UE_LOG(Esmini, Warning, TEXT("Unknown object type( %s ) for %s"), *typeName, *objName);
				break;
			}
			}
		}
	}
}
void AEsminiManager::SendWeatherUpdate()
{
	FString WeatherPath = UStandaloneWeatherParser::FindMatchingWeatherFile(Path);
	FStandaloneWeatherData Weather;
	FString Error;
	bool bGotWeather = false;
	if (!WeatherPath.IsEmpty()) {
		bGotWeather = UStandaloneWeatherParser::ParseEnvironmentFile(WeatherPath, Weather, Error);
	}
	else {
		FXmlFile XmlFile(
			Path,
			EConstructMethod::ConstructFromFile);
		if (!XmlFile.IsValid())
		{
			UE_LOG(Esmini, Error,
				TEXT("SendWeatherUpdate: Could not parse XML file '%s': %s"),
				*Path,
				*XmlFile.GetLastError());


		}
		const FXmlNode* RootNode = XmlFile.GetRootNode();

		const FXmlNode* StoryboardNode =
			UStandaloneWeatherParser::FindFirstNode(RootNode, TEXT("Storyboard"));
		const FXmlNode* InitNode = StoryboardNode ?
			UStandaloneWeatherParser::FindFirstNode(StoryboardNode, TEXT("Init")) : nullptr;
		const FXmlNode* ActionsNode = InitNode ?
			UStandaloneWeatherParser::FindFirstNode(InitNode, TEXT("Actions")) : nullptr;
		const FXmlNode* GlobalActionNode = ActionsNode ?
			UStandaloneWeatherParser::FindFirstNode(ActionsNode, TEXT("GlobalAction")) : nullptr;
		const FXmlNode* EnvironmentActionNode = GlobalActionNode ?
			UStandaloneWeatherParser::FindFirstNode(GlobalActionNode, TEXT("EnvironmentAction")) : nullptr;
		const FXmlNode* EnvironmentNode = EnvironmentActionNode ?
			UStandaloneWeatherParser::FindFirstNode(EnvironmentActionNode, TEXT("Environment")) : nullptr;
		bGotWeather = UStandaloneWeatherParser::ParseEnvironmentNode(EnvironmentNode, Weather, Error);
	}
	if (bGotWeather) {
		UE_LOG(Esmini, Verbose, TEXT(
			"Temperature: %.1f C | "
			"Pressure: %.1f hPa | "
			"Visibility: %.0f m | "
			"Rain/Snow: %s %.1f mm/h"),
			Weather.TemperatureCelsius, Weather.AtmosphericPressureHectoPascal,
			Weather.VisualRangeMeters, *Weather.PrecipitationType,
			Weather.PrecipitationIntensityMmPerHour);
		FDateTime Dt;
		if (!FDateTime::ParseIso8601(*Weather.TimeOfDayDateTime, Dt)) {
			Dt = -1;
			UE_LOG(Esmini, Warning,
				TEXT("Cant convret %s to FDateTime"),
				*Weather.TimeOfDayDateTime
			);
		}
		FWeatherData Data;
		Data.TimeOfDayInfo.bHasTimeOfDay = Weather.bHasTimeOfDay;
		Data.TimeOfDayInfo.ScenarioDateTime = Weather.TimeOfDayDateTime;
		Data.TimeOfDayInfo.TimeOfDayInSeconds = Dt.GetTimeOfDay().GetTotalSeconds();
		Data.WeatherState.bWeatherEffectEnabled = false;
		Data.WeatherState.ActiveWeatherType = NAME_None;
		Data.WeatherState.bHasMOR = Weather.bHasFog;
		Data.WeatherState.MOR = Weather.VisualRangeMeters;
		Data.WeatherState.bHasPrecipitation = Weather.bHasPrecipitation;
		if (Weather.bHasPrecipitation) {
			if (Weather.PrecipitationType.Equals(TEXT("Rain"), ESearchCase::IgnoreCase)) {
				Data.WeatherState.PrecipitationType = GemstarPrecipitationType::Rain;

			}
			else if (Weather.PrecipitationType.Equals(TEXT("Snow"), ESearchCase::IgnoreCase)) {
				Data.WeatherState.PrecipitationType = GemstarPrecipitationType::Snow;

			}
			else if (Weather.PrecipitationType.Equals(TEXT("Dry"), ESearchCase::IgnoreCase)) {
				Data.WeatherState.PrecipitationType = GemstarPrecipitationType::Dry;
			}
		}
		Data.WeatherState.PrecipitationIntensity = Weather.PrecipitationIntensityMmPerHour;
		Data.WeatherState.bHasTemperature = Weather.bHasTemperature;
		Data.WeatherState.Temperature = Weather.TemperatureKelvin;
		Data.WeatherState.bHasPressure = Weather.bHasAtmosphericPressure;
		Data.WeatherState.AtmosphericPressure = Weather.AtmosphericPressurePascal;
		Data.WeatherState.bHasWind = Weather.bHasWind;
		Data.WeatherState.WindSpeed = Weather.WindSpeedMetersPerSecond;
		Data.WeatherState.WindDirection = Weather.WindDirectionRadians;
		Data.WeatherState.bHasSun = false;
		Data.WeatherState.SunIlluminance = 0.0;
		Data.WeatherState.SunAzimuth = 0.0;
		Data.WeatherState.SunElevation = 0.0;
		Data.WeatherState.bHasFractionalCloudCover = true;
		Data.WeatherState.FractionalCloudCover = Weather.FractionalCloudCover_normalized * 100;
		for (auto& WeatherEffectActor : WeatherEffectActorsToInform) {
			IWeatherDataUpdate::Execute_WeatherDataUpdate(WeatherEffectActor.Actor, Data, 0);
		}
	}
	else
	{
		UE_LOG(Esmini, Error, TEXT("Weather parsing failed: %s"), *Error);
	}
}


FTransform AEsminiManager::TranslateToUnrealWorldPosition(const FScenarioObjectState& obj)
{
	FVector PosUe(
		obj.x * 100,
		-obj.y * 100,
		obj.z * 100
	);

	//fix calculation errors in following algorithms
	if (PosUe.Z == NAN) {
		PosUe.Z = 0;
	}

	float Pitch = FMath::RadiansToDegrees(obj.p);
	float Yaw = -FMath::RadiansToDegrees(obj.h);
	float Roll = FMath::RadiansToDegrees(obj.r);

	FTransform Transform;
	Transform.SetLocation(CoordinateSystemOffset.GetLocation() + PosUe);
	Transform.SetRotation(FRotator(Pitch, Yaw, Roll).Quaternion());

	if (bDebug) {
		DrawDebugCoordinateSystem(GetWorld(), PosUe, FRotator(Pitch, Yaw, Roll), 500.f, false, 10.f);
		DrawDebugCoordinateSystem(GetWorld(), Transform.GetLocation(), Transform.GetRotation().Rotator(), 500.f, false, 10.f);
	}
	return Transform;
}

FString AEsminiManager::GetOpenSCENARIORootDirectory()
{
	const FString ProjectDir = FPaths::ProjectDir();
	FString OscDir = FPaths::Combine(
		ProjectDir,
		TEXT("Simulation"),
		TEXT("OpenSCENARIO"));
	OscDir = FPaths::ConvertRelativePathToFull(OscDir);
	FPaths::NormalizeDirectoryName(OscDir);
	return OscDir;
}


void AEsminiManager::VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) {
	if (data.Provider.IsEqual(PROVIDER_NAME)) {
		//skip, use for testing
		UE_LOG(Esmini, Warning, TEXT("Should not receive own data %s "), *data.Name.ToString());
	}
	else {
		//todo for co-simulation purposes
	}
}

void AEsminiManager::RegisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>()) {
		VehicleActorsToInform.AddUnique({ vehicleDataReceiver, 0 });
	}
	else {
		UE_LOG(Esmini, Log, TEXT("Tried to register an actor without UEsminiVehicleDataUpdate."));
	}
}

void AEsminiManager::UnregisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>()) {
		VehicleActorsToInform.Remove({ vehicleDataReceiver, 0 });
	}
}

