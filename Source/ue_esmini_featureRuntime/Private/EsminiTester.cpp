// Fill out your copyright notice in the Description page of Project Settings.


#include "EsminiTester.h"

// Sets default values
AEsminiTester::AEsminiTester()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEsminiTester::BeginPlay()
{
	Super::BeginPlay();
	if (esminiManager == nullptr) {
		UE_LOG(LogTemp, Log, TEXT("No esmini Actor defined. Test failed."));
	}
	else {
		IVehicleDataDispatcher::Execute_RegisterVehicleDataReceiver(esminiManager, this);
	}
	TMap<FString, FInnerConfig> config;
	FInnerConfig inner;
	inner.Entries.Add("Path", "Resources/cut-in/cut-in.xosc");
	config.Add("esmini", inner);
	esminiManager->Configure(config, nullptr);
}

// Called every frame
void AEsminiTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (esminiManager == nullptr) {
		UE_LOG(LogTemp, Log, TEXT("No SUMO Actor defined. Test failed."));
	}
}

void AEsminiTester::VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) {
	UE_LOG(LogTemp, Log,
		TEXT("VehicleDataUpdateData { Provider: %s, Name: %s, Position: %s, Speed: %.2f, Accel: %.2f, Signals: %d, VehicleClass: %s, BoundingBox: %s }"),
		*data.Provider.ToString(),
		*data.Name.ToString(),
		*data.Position.ToString(),
		data.speed,
		data.accel,
		data.signals,
		*data.VehicleClass.ToString(),
		*data.BoundingBox.ToString()
	);
}

