#pragma once
#include "cashgen.h"
#include "ProceduralMeshComponent.h"
#include "Meshdata.generated.h"

/** Defines the data required for a single procedural mesh section */
USTRUCT()
struct FMeshData
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Data Struct")
	TArray<FVector> MyVertices;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Data Struct")
	TArray<int32> MyTriangles;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Data Struct")
	TArray<FVector> MyNormals;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Data Struct")
	TArray<FVector2D> MyUV0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Data Struct")
	TArray<FColor> MyVertexColors;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Data Struct")
	TArray<FProcMeshTangent> MyTangents;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Data Struct")
	TArray<FVector> MyHeightMap;
};




