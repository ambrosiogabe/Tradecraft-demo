// Fill out your copyright notice in the Description page of Project Settings.

#include "MinecraftWorld.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/UObjectGlobals.h"
#include "Camera/CameraComponent.h"
#include "Misc/DateTime.h"


// Sets default values
AMinecraftWorld::AMinecraftWorld()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMinecraftWorld::BeginPlay()
{
	Super::BeginPlay();
	ItemCounts.Init(0, 54);
	ItemIds.Init(0, 54);

	// Create master file if needed ------------------------------------------------------------------------------------------------
	UGameSaverAndLoader* SavedGames = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(TEXT("All_Saved_Games"), 0))
	{
		SavedGames = Cast<UGameSaverAndLoader>(UGameplayStatics::LoadGameFromSlot(TEXT("All_Saved_Games"), 0));
		UE_LOG(LogTemp, Warning, TEXT("Loaded existing save game loader master file."));
	}
	else
	{
		SavedGames = Cast<UGameSaverAndLoader>(UGameplayStatics::CreateSaveGameObject(UGameSaverAndLoader::StaticClass()));
		UGameplayStatics::SaveGameToSlot(SavedGames, TEXT("All_Saved_Games"), 0);
		UE_LOG(LogTemp, Warning, TEXT("Created new save game and load game master file."));
	}
	// ------------------------------------------------------------------------------------------------------------------------------

	// Declare stuff for seed -----------------------------------
	FDateTime Date = FDateTime::Now();
	int32 year = Date.GetYear();
	int32 day = Date.GetDayOfYear();
	int32 hour = Date.GetHour();
	int32 minute = Date.GetMinute();
	int32 second = Date.GetSecond();
	// Declare stuff for seed -----------------------------------

	// Delete Games if we need to -------------------------------
	if (Worlds_To_Delete.Num() != 0) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Deleting pending worlds."));		
		for (int i = 0; i < Worlds_To_Delete.Num(); i++) 
		{
			FString curName = Worlds_To_Delete[i];
			FString SavedDirectoryName = FPaths::Combine(FPaths::ProjectSavedDir(), FString("SaveGames"), curName);
			FString SaveFileName = FPaths::Combine(FPaths::ProjectSavedDir(), FString("SaveGames"), curName.Append(".sav"));
			UE_LOG(LogTemp, Warning, TEXT("Save File Name: %s"), *SaveFileName);
			UE_LOG(LogTemp, Warning, TEXT("Saved Directory Name: %s"), *SavedDirectoryName);
			SavedGames->DeleteFile(SaveFileName);
			SavedGames->DeleteDirectory(SavedDirectoryName);
		}
		
		int32 curIndex = 0;
		while (curIndex < Worlds_To_Delete.Num()) 
		{
			FString name = Worlds_To_Delete[curIndex];
			SavedGames->AllSavedGames.Remove(name);
			curIndex++;
		}
		Worlds_To_Delete.Empty();
		UGameplayStatics::SaveGameToSlot(SavedGames, TEXT("All_Saved_Games"), 0);
	}
	// -----------------------------------------------------------------------------------------------------------------------------

	// If we don't find a saved game, create the world, otherwise load the game ----------------------------------------------------------------------------------------------------
	if (!UGameplayStatics::DoesSaveGameExist(WorldName, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("New saved game created, add to the world."));
		if (seed == 0)
			seed = ((year - 2018) * 356) + (day * 24) + (hour * 60) + (minute * 60) + second;

		SaveGameInstance = Cast<UGameSaverAndLoader>(UGameplayStatics::CreateSaveGameObject(UGameSaverAndLoader::StaticClass()));
		SaveGameInstance->SaveSlotName = WorldName;
		SaveGameInstance->SavedSeed = seed;
		UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName, SaveGameInstance->UserIndex);
		SavedGames->AllSavedGames.Add(SaveGameInstance->SaveSlotName);
		UGameplayStatics::SaveGameToSlot(SavedGames, TEXT("All_Saved_Games"), 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Game loaded."));
		SaveGameInstance = Cast<UGameSaverAndLoader>(UGameplayStatics::LoadGameFromSlot(WorldName, 0));
		seed = SaveGameInstance->SavedSeed;
		IsSavedWorld = true;
	}
	// Load Game if we find one ------------------------------------------------------------------------------------------------------------------------------------------------

	WorldDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), FString("SaveGames"), WorldName);
	UE_LOG(LogTemp, Warning, TEXT("World Directory: %s"), *WorldDirectory);
	SaveGameInstance->VerifyOrCreateDirectory(WorldDirectory);

	Block_Health_Values.Init(0, Block_Props.Num());
	for (int i = 0; i < Block_Props.Num(); i++) 
	{
		Block_Health_Values[i] = Block_Props[i].Current_Health;
	}

	Player = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (!Player)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find player."));
	}

	LastBuildPosition = Player->GetActorLocation();

	FVector FirstChunkPos = FVector((int)((LastBuildPosition.X - 1) / (ChunkWidth * 100)), (int)(LastBuildPosition.Y / (ChunkWidth * 100)), (int)(LastBuildPosition.Z / (ChunkWidth * 100)));

	AChunk* Cube = GetWorld()->SpawnActor<AChunk>();
	BuildNearPlayer();

	// Set player's location if we are loading a game and inventory etc.---------------------------
	// Do it after the world has been built, so that the player does not---------------------------
	// fall through the world. --------------------------------------------------------------------
	if (IsSavedWorld)
	{
		Player->SetActorLocationAndRotation(SaveGameInstance->PlayerPosition, SaveGameInstance->PlayerRotation);
		UE_LOG(LogTemp, Warning, TEXT("Player Rotation: %s"), *SaveGameInstance->PlayerRotation.ToString());
	}
	else
	{
		SaveGameInstance->PlayerPosition = Player->GetActorLocation();
		SaveGameInstance->PlayerRotation = Player->GetActorRotation();
		UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName, SaveGameInstance->UserIndex);
	}
	//---------------------------------------------------------------------------------------------

	WorldIsLoaded = true;
}

bool AMinecraftWorld::IsWorldLoaded() {
	return WorldIsLoaded;
}

void AMinecraftWorld::SetWorldLoaded(bool loaded) {
	WorldIsLoaded = loaded;
}

// Called every frame
void AMinecraftWorld::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector Movement = LastBuildPosition - Player->GetActorLocation();

	if (FMath::Sqrt(Movement.X * Movement.X + Movement.Y * Movement.Y) > (ChunkWidth * 100))
	{
		LastBuildPosition = Player->GetActorLocation();
		BuildNearPlayer();
	}
}

void AMinecraftWorld::BuildChunkAt(FVector pos)
{
	FString chunkName = AMinecraftWorld::BuildChunkName(pos);
	AChunk* Chunk;

	if (!Chunks.Contains(chunkName) && !SaveGameInstance->CheckIfFileExists(WorldDirectory, chunkName))
	{
		Chunk = GetWorld()->SpawnActor<AChunk>();
		FVector ChunkPos = FVector(pos.X * Chunk->WidthOfChunk * Chunk->VoxelWidth, pos.Y * Chunk->WidthOfChunk * Chunk->VoxelWidth, -Chunk->VoxelWidth * (Chunk->HeightOfChunk / 2));
		Chunk->SetLocation(ChunkPos, seed);
		Chunk->SetChunkMaterials(Materials);
		Chunk->SetBlockHealthValues(Block_Health_Values);
		Chunk->MakeOwner(this);


		Chunk->GenerateChunkInWorld();
		Chunks.Add(chunkName, Chunk);
	}
	else if (SaveGameInstance->CheckIfFileExists(WorldDirectory, chunkName) && !Chunks.Contains(chunkName))
	{
		Chunk = GetWorld()->SpawnActor<AChunk>();
		FVector ChunkPos = FVector(pos.X * Chunk->WidthOfChunk * Chunk->VoxelWidth, pos.Y * Chunk->WidthOfChunk * Chunk->VoxelWidth, -Chunk->VoxelWidth * (Chunk->HeightOfChunk / 2));
		Chunk->SetLocation(ChunkPos, seed);
		Chunk->SetChunkMaterials(Materials);
		Chunk->SetBlockHealthValues(Block_Health_Values);
		Chunk->MakeOwner(this);

		TArray<int32> ChunkIds;
		FString PathToSaveData = FPaths::Combine(WorldDirectory, chunkName);
		SaveGameInstance->LoadGameDataFromFileCompressed(PathToSaveData, ChunkIds);
		Chunk->LoadChunkValues(ChunkIds);
		Chunk->GenerateLoadedChunkInWorld();

		if(Chunk)
			Chunks.Add(chunkName, Chunk);
	}
	else
		Chunk = Chunks[chunkName];

	if (!Chunk)
		UE_LOG(LogTemp, Error, TEXT("CHUNK NOT FOUND OR CREATED!!"));
}

void AMinecraftWorld::RemoveOldChunks()
{
	removingChunks = true;
	for (int i = 0; i < ToRemove.Num(); i++)
	{
		FString name = ToRemove[i];
		AChunk* ChunkToRemove = nullptr;
		if (Chunks.Contains(name))
		{
			if (Chunks[name] == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("Houston we have an error."));
				break;
			}
			else
				ChunkToRemove = Chunks[name];
			
			if (!ChunkToRemove->IsPendingKill())
			{
				TArray<int32> ChunkIds;
				for (int i = 0; i < ChunkToRemove->ChunkData.Num(); i++) 
				{
					ChunkIds.Add(ChunkToRemove->ChunkData[i].id);
				}
				FString Path = FPaths::Combine(WorldDirectory, name);
				SaveGameInstance->SaveGameDataToFileCompressed(Path, ChunkIds);

				Chunks.Remove(name);
				ChunkToRemove->Destroy();
				if (Chunks.Contains(name))
					UE_LOG(LogTemp, Warning, TEXT("Does chunk still exist? Apparently"));
			}
			else
				UE_LOG(LogTemp, Warning, TEXT("Chunk is already pending to be destroyed."));
		}
	}
	ToRemove.Empty();
	removingChunks = false;
}

void AMinecraftWorld::RecursivelyBuildWorld(FVector pos, int32 radius)
{
	int32 NXRange = pos.X - (radius / 2);
	int32 NYRange = pos.Y - (radius / 2);

	for (int32 x = NXRange; x < NXRange + radius; x++)
	{
		for (int32 y = NYRange; y < NYRange + radius; y++)
		{
			BuildChunkAt(FVector(x, y, -16));
		}
	}
}

void AMinecraftWorld::BuildNearPlayer()
{
	FVector PlayerPos = Player->GetActorLocation();

	FVector PlayerChunkPos = FVector((int)((PlayerPos.X - 1) / (ChunkWidth * 100)), (int)(PlayerPos.Y / (ChunkWidth * 100)), (int)(PlayerPos.Z / (ChunkWidth * 100)));
	RecursivelyBuildWorld(PlayerChunkPos, ChunkRange);

	if (!removingChunks)
	{
		for (auto& Elem : Chunks)
		{
			FString chunkName = Elem.Key;

			AChunk* Chunk = Elem.Value;
			FVector PlayerPos = Player->GetActorLocation();
			FVector ChunkPos = Chunk->GetActorLocation();

			float xVal = (PlayerPos.X - ChunkPos.X) * (PlayerPos.X - ChunkPos.X);
			float yVal = (PlayerPos.Y - ChunkPos.Y) * (PlayerPos.Y - ChunkPos.Y);

			if (FMath::Sqrt(xVal + yVal) > ((ChunkRange * Chunk->WidthOfChunk * 100)))
			{
				ToRemove.Add(chunkName);
			}
		}

		RemoveOldChunks();
	}
}

FString AMinecraftWorld::BuildChunkName(FVector pos)
{
	FString name = "";
	name = "Chunk_" + FString::SanitizeFloat(pos.X) + "_" + FString::SanitizeFloat(pos.Y) + "_" + FString::SanitizeFloat(pos.Z);
	return name;
}

int32 AMinecraftWorld::BreakBlock(FVector pos)
{
	int32 BlockBroken = BreakOrAddBlock(pos, false, 0);
	return BlockBroken;
}

void AMinecraftWorld::AddBlock(FVector pos, int32 id)
{
	BreakOrAddBlock(pos, true, id);
}

int32 AMinecraftWorld::DealDamage(FVector pos, int32 damage) 
{
	int32 BlockX = FMath::CeilToFloat((pos.X / 100) - 0.5);
	int32 BlockY = FMath::CeilToFloat((pos.Y / 100) - 0.5);
	int32 BlockZ = FMath::CeilToFloat((pos.Z / 100) - 0.5);

	int32 ChunkCalcX = FMath::FloorToFloat(FMath::CeilToFloat((pos.X / 100) - 0.5) / ChunkWidth);
	int32 ChunkCalcY = FMath::FloorToFloat(FMath::CeilToFloat((pos.Y / 100) - 0.5) / ChunkWidth);

	FString name = BuildChunkName(FVector(ChunkCalcX, ChunkCalcY, -16.0));
	AChunk* HitChunk = nullptr;

	if (AMinecraftWorld::Chunks.Contains(name))
	{
		HitChunk = AMinecraftWorld::Chunks[name];
		int32 ChunkXIndex = (int)(HitChunk->GetActorLocation().X / 100);
		int32 ChunkYIndex = (int)(HitChunk->GetActorLocation().Y / 100);

		BlockZ += (int)(HitChunk->HeightOfChunk / 2);
		BlockY = FMath::Abs(BlockY - ChunkYIndex) + 1;
		BlockX = FMath::Abs(BlockX - ChunkXIndex) + 1;

		int32 health = HitChunk->DealDamage(BlockX, BlockY, BlockZ, damage);
		return health;
	}
	return 1;
}

int32 AMinecraftWorld::BreakOrAddBlock(FVector pos, bool addBlock, int32 id)
{
	int32 BlockX = FMath::CeilToFloat((pos.X / 100) - 0.5);
	int32 BlockY = FMath::CeilToFloat((pos.Y / 100) - 0.5);
	int32 BlockZ = FMath::CeilToFloat((pos.Z / 100) - 0.5);

	int32 ChunkCalcX = FMath::FloorToFloat(FMath::CeilToFloat((pos.X / 100) - 0.5) / ChunkWidth);
	int32 ChunkCalcY = FMath::FloorToFloat(FMath::CeilToFloat((pos.Y / 100) - 0.5) / ChunkWidth);

	FString name = BuildChunkName(FVector(ChunkCalcX, ChunkCalcY, -16.0));
	AChunk* HitChunk = nullptr;

	if (AMinecraftWorld::Chunks.Contains(name))
	{
		HitChunk = AMinecraftWorld::Chunks[name];
		int32 ChunkXIndex = (int)(HitChunk->GetActorLocation().X / 100);
		int32 ChunkYIndex = (int)(HitChunk->GetActorLocation().Y / 100);

		BlockZ += (int)(HitChunk->HeightOfChunk / 2);
		BlockY = FMath::Abs(BlockY - ChunkYIndex) + 1;
		BlockX = FMath::Abs(BlockX - ChunkXIndex) + 1;

		// This is all the possible chunks I may need to update
		int32 FrontChunkCalcX = FMath::FloorToFloat(FMath::CeilToFloat(((pos.X + 100) / 100) - 0.5) / ChunkWidth);
		int32 BackChunkCalcX = FMath::FloorToFloat(FMath::CeilToFloat(((pos.X - 100) / 100) - 0.5) / ChunkWidth);
		int32 RightChunkCalcY = FMath::FloorToFloat(FMath::CeilToFloat(((pos.Y + 100) / 100) - 0.5) / ChunkWidth);
		int32 LeftChunkCalcY = FMath::FloorToFloat(FMath::CeilToFloat(((pos.Y - 100) / 100) - 0.5) / ChunkWidth);
		AChunk* FrontChunk = AMinecraftWorld::Chunks[BuildChunkName(FVector(FrontChunkCalcX, ChunkCalcY, -16.0))];
		AChunk* BackChunk = AMinecraftWorld::Chunks[BuildChunkName(FVector(BackChunkCalcX, ChunkCalcY, -16.0))];
		AChunk* RightChunk = AMinecraftWorld::Chunks[BuildChunkName(FVector(ChunkCalcX, RightChunkCalcY, -16.0))];
		AChunk* LeftChunk = AMinecraftWorld::Chunks[BuildChunkName(FVector(ChunkCalcX, LeftChunkCalcY, -16.0))];

		if (BlockX == ChunkWidth)
			FrontChunk->BreakBlock(0, BlockY, BlockZ);
		else if (BlockX == 1)
			BackChunk->BreakBlock(ChunkWidth + 1, BlockY, BlockZ);

		if (BlockY == ChunkWidth)
			RightChunk->BreakBlock(BlockX, 0, BlockZ);
		else if (BlockY == 1)
			LeftChunk->BreakBlock(BlockX, ChunkWidth + 1, BlockZ);

		if (addBlock)
			HitChunk->AddBlock(BlockX, BlockY, BlockZ, id);
		else
		{
			int32 newId = HitChunk->BreakBlock(BlockX, BlockY, BlockZ);
			return newId;
		}
	}
	return 0;
}

int32 AMinecraftWorld::GetTotalHealth(FVector pos) 
{
	int32 id = GetBlockAtPos(pos);

	if (id != 0)
	{
		return Block_Props[id].Current_Health;
	}
	return 1;
}

int32 AMinecraftWorld::GetBlockAtPos(FVector pos)
{
	int32 BlockX = FMath::CeilToFloat((pos.X / 100) - 0.5);
	int32 BlockY = FMath::CeilToFloat((pos.Y / 100) - 0.5);
	int32 BlockZ = FMath::CeilToFloat((pos.Z / 100) - 0.5);

	int32 ChunkCalcX = FMath::FloorToFloat(FMath::CeilToFloat((pos.X / 100) - 0.5) / ChunkWidth);
	int32 ChunkCalcY = FMath::FloorToFloat(FMath::CeilToFloat((pos.Y / 100) - 0.5) / ChunkWidth);

	FString name = BuildChunkName(FVector(ChunkCalcX, ChunkCalcY, -16.0));
	AChunk* HitChunk = nullptr;

	if (AMinecraftWorld::Chunks.Contains(name))
	{
		HitChunk = AMinecraftWorld::Chunks[name];
		int32 ChunkXIndex = (int)(HitChunk->GetActorLocation().X / 100);
		int32 ChunkYIndex = (int)(HitChunk->GetActorLocation().Y / 100);

		BlockZ += (int)(HitChunk->HeightOfChunk / 2);
		BlockY = FMath::Abs(BlockY - ChunkYIndex) + 1;
		BlockX = FMath::Abs(BlockX - ChunkXIndex) + 1;

		int32 arrayPos = BlockZ + (BlockY * HitChunk->HeightOfChunk) + (BlockX * HitChunk->HeightOfChunk * HitChunk->WidthOfChunkExt);
		int32 id = HitChunk->GetBlockId(arrayPos);
		return id;
	}
	return 0;
}

void AMinecraftWorld::ExitAndSave(TArray<int32> ItemIds, TArray<int32> ItemCounts)
{
	SaveGameInstance->PlayerPosition = Player->GetActorLocation();
	SaveGameInstance->PlayerRotation = Player->GetActorRotation();
	// TODO figure out a way to save camera rotation
	//SaveGameInstance->CameraRotation = GetComponentByClass<UCameraComponent>().GetActorRotation();
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName, SaveGameInstance->UserIndex);

	for (auto& i : Chunks) 
	{
		AChunk* CurrentChunk = i.Value;
		FString CurrentChunkName = i.Key;
		FString PathToChunkName = FPaths::Combine(WorldDirectory, CurrentChunkName);

		TArray<int32> ChunkIds;
		for (int i = 0; i < CurrentChunk->ChunkData.Num(); i++) 
		{
			ChunkIds.Add(CurrentChunk->ChunkData[i].id);
		}

		SaveGameInstance->SaveGameDataToFileCompressed(PathToChunkName, ChunkIds);
	}

	FString PlayerDat = FPaths::Combine(WorldDirectory, FString("Player"));
	SaveGameInstance->SaveGameDataToFileCompressed(PlayerDat, ItemIds, ItemCounts);
}

TArray<int32> AMinecraftWorld::LoadItemCounts() 
{ 
	return ItemCounts;
}
TArray<int32> AMinecraftWorld::LoadItemIds() 
{ 
	FString PlayerDat = FPaths::Combine(WorldDirectory, FString("Player"));
	UE_LOG(LogTemp, Warning, TEXT("Player Dat path: %s"), *PlayerDat);

	// We need to load both since we saved them into the same file, but we're only going to return the
	// item counts so that we are only returning one variable to the blueprint asking for the info.
	bool exists = SaveGameInstance->LoadGameDataFromFileCompressed(PlayerDat, ItemIds, ItemCounts);

	return ItemIds;
}