#include "Mesh/ARPGProceduralMeshActor.h"

AARPGProceduralMeshActor::AARPGProceduralMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;
	ProceduralMesh->bUseAsyncCooking = true;
}

void AARPGProceduralMeshActor::GeneratePlatform(float Width, float Depth, float Height, int32 SubdivisionsX, int32 SubdivisionsY)
{
	SubdivisionsX = FMath::Max(1, SubdivisionsX);
	SubdivisionsY = FMath::Max(1, SubdivisionsY);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	const float HalfW = Width * 0.5f;
	const float HalfD = Depth * 0.5f;

	const int32 VertsX = SubdivisionsX + 1;
	const int32 VertsY = SubdivisionsY + 1;

	// Top face
	for (int32 Y = 0; Y <= SubdivisionsY; ++Y)
	{
		for (int32 X = 0; X <= SubdivisionsX; ++X)
		{
			const float PosX = -HalfW + (Width * X / SubdivisionsX);
			const float PosY = -HalfD + (Depth * Y / SubdivisionsY);
			Vertices.Add(FVector(PosX, PosY, Height));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(static_cast<float>(X) / SubdivisionsX, static_cast<float>(Y) / SubdivisionsY));
		}
	}

	for (int32 Y = 0; Y < SubdivisionsY; ++Y)
	{
		for (int32 X = 0; X < SubdivisionsX; ++X)
		{
			const int32 BL = Y * VertsX + X;
			const int32 BR = BL + 1;
			const int32 TL = BL + VertsX;
			const int32 TR = TL + 1;

			Triangles.Append({BL, TL, BR, BR, TL, TR});
		}
	}

	// Bottom face
	const int32 BottomOffset = Vertices.Num();
	for (int32 Y = 0; Y <= SubdivisionsY; ++Y)
	{
		for (int32 X = 0; X <= SubdivisionsX; ++X)
		{
			const float PosX = -HalfW + (Width * X / SubdivisionsX);
			const float PosY = -HalfD + (Depth * Y / SubdivisionsY);
			Vertices.Add(FVector(PosX, PosY, 0.f));
			Normals.Add(-FVector::UpVector);
			UVs.Add(FVector2D(static_cast<float>(X) / SubdivisionsX, static_cast<float>(Y) / SubdivisionsY));
		}
	}

	for (int32 Y = 0; Y < SubdivisionsY; ++Y)
	{
		for (int32 X = 0; X < SubdivisionsX; ++X)
		{
			const int32 BL = BottomOffset + Y * VertsX + X;
			const int32 BR = BL + 1;
			const int32 TL = BL + VertsX;
			const int32 TR = TL + 1;

			Triangles.Append({BL, BR, TL, BR, TR, TL});
		}
	}

	// Side faces — 4 walls connecting top and bottom
	auto AddSideQuad = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal)
	{
		const int32 Base = Vertices.Num();
		Vertices.Append({A, B, C, D});
		Normals.Append({Normal, Normal, Normal, Normal});
		UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1), FVector2D(1, 1)});
		Triangles.Append({Base, Base + 2, Base + 1, Base + 1, Base + 2, Base + 3});
	};

	// Front (Y = -HalfD)
	AddSideQuad(FVector(-HalfW, -HalfD, Height), FVector(HalfW, -HalfD, Height),
		FVector(-HalfW, -HalfD, 0), FVector(HalfW, -HalfD, 0), FVector(0, -1, 0));
	// Back (Y = HalfD)
	AddSideQuad(FVector(HalfW, HalfD, Height), FVector(-HalfW, HalfD, Height),
		FVector(HalfW, HalfD, 0), FVector(-HalfW, HalfD, 0), FVector(0, 1, 0));
	// Left (X = -HalfW)
	AddSideQuad(FVector(-HalfW, HalfD, Height), FVector(-HalfW, -HalfD, Height),
		FVector(-HalfW, HalfD, 0), FVector(-HalfW, -HalfD, 0), FVector(-1, 0, 0));
	// Right (X = HalfW)
	AddSideQuad(FVector(HalfW, -HalfD, Height), FVector(HalfW, HalfD, Height),
		FVector(HalfW, -HalfD, 0), FVector(HalfW, HalfD, 0), FVector(1, 0, 0));

	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), bCreateCollision);
	ApplyMaterial(0);
}

void AARPGProceduralMeshActor::GenerateWall(float Width, float Height, float Thickness)
{
	// A wall is a thin platform standing upright — reuse the box generation pattern
	const float HalfW = Width * 0.5f;
	const float HalfT = Thickness * 0.5f;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	auto AddQuad = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal)
	{
		const int32 Base = Vertices.Num();
		Vertices.Append({A, B, C, D});
		Normals.Append({Normal, Normal, Normal, Normal});
		const float UScale = FVector::Dist(A, B) / 100.f;
		const float VScale = FVector::Dist(A, C) / 100.f;
		UVs.Append({FVector2D(0, 0), FVector2D(UScale, 0), FVector2D(0, VScale), FVector2D(UScale, VScale)});
		Triangles.Append({Base, Base + 2, Base + 1, Base + 1, Base + 2, Base + 3});
	};

	// Front face (Y = -HalfT)
	AddQuad(FVector(-HalfW, -HalfT, Height), FVector(HalfW, -HalfT, Height),
		FVector(-HalfW, -HalfT, 0), FVector(HalfW, -HalfT, 0), FVector(0, -1, 0));
	// Back face (Y = HalfT)
	AddQuad(FVector(HalfW, HalfT, Height), FVector(-HalfW, HalfT, Height),
		FVector(HalfW, HalfT, 0), FVector(-HalfW, HalfT, 0), FVector(0, 1, 0));
	// Top face
	AddQuad(FVector(-HalfW, -HalfT, Height), FVector(HalfW, -HalfT, Height),
		FVector(-HalfW, HalfT, Height), FVector(HalfW, HalfT, Height), FVector::UpVector);
	// Bottom face
	AddQuad(FVector(-HalfW, HalfT, 0), FVector(HalfW, HalfT, 0),
		FVector(-HalfW, -HalfT, 0), FVector(HalfW, -HalfT, 0), -FVector::UpVector);
	// Left face (X = -HalfW)
	AddQuad(FVector(-HalfW, HalfT, Height), FVector(-HalfW, -HalfT, Height),
		FVector(-HalfW, HalfT, 0), FVector(-HalfW, -HalfT, 0), FVector(-1, 0, 0));
	// Right face (X = HalfW)
	AddQuad(FVector(HalfW, -HalfT, Height), FVector(HalfW, HalfT, Height),
		FVector(HalfW, -HalfT, 0), FVector(HalfW, HalfT, 0), FVector(1, 0, 0));

	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), bCreateCollision);
	ApplyMaterial(0);
}

void AARPGProceduralMeshActor::GenerateRamp(float Width, float Depth, float Height)
{
	const float HalfW = Width * 0.5f;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	// Slope face (top surface)
	const FVector SlopeNormal = FVector(0, -Height, Depth).GetSafeNormal();
	{
		const int32 Base = Vertices.Num();
		Vertices.Add(FVector(-HalfW, 0, 0));       // front-left bottom
		Vertices.Add(FVector(HalfW, 0, 0));         // front-right bottom
		Vertices.Add(FVector(-HalfW, Depth, Height)); // back-left top
		Vertices.Add(FVector(HalfW, Depth, Height));  // back-right top
		Normals.Append({SlopeNormal, SlopeNormal, SlopeNormal, SlopeNormal});
		UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1), FVector2D(1, 1)});
		Triangles.Append({Base, Base + 2, Base + 1, Base + 1, Base + 2, Base + 3});
	}

	// Bottom face
	{
		const int32 Base = Vertices.Num();
		Vertices.Add(FVector(-HalfW, 0, 0));
		Vertices.Add(FVector(HalfW, 0, 0));
		Vertices.Add(FVector(-HalfW, Depth, 0));
		Vertices.Add(FVector(HalfW, Depth, 0));
		const FVector N = -FVector::UpVector;
		Normals.Append({N, N, N, N});
		UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1), FVector2D(1, 1)});
		Triangles.Append({Base, Base + 1, Base + 2, Base + 1, Base + 3, Base + 2});
	}

	// Back wall (vertical face at high end)
	{
		const int32 Base = Vertices.Num();
		Vertices.Add(FVector(-HalfW, Depth, Height));
		Vertices.Add(FVector(HalfW, Depth, Height));
		Vertices.Add(FVector(-HalfW, Depth, 0));
		Vertices.Add(FVector(HalfW, Depth, 0));
		const FVector N(0, 1, 0);
		Normals.Append({N, N, N, N});
		UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1), FVector2D(1, 1)});
		Triangles.Append({Base, Base + 2, Base + 1, Base + 1, Base + 2, Base + 3});
	}

	// Left triangle
	{
		const int32 Base = Vertices.Num();
		Vertices.Add(FVector(-HalfW, 0, 0));
		Vertices.Add(FVector(-HalfW, Depth, 0));
		Vertices.Add(FVector(-HalfW, Depth, Height));
		const FVector N(-1, 0, 0);
		Normals.Append({N, N, N});
		UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1)});
		Triangles.Append({Base, Base + 1, Base + 2});
	}

	// Right triangle
	{
		const int32 Base = Vertices.Num();
		Vertices.Add(FVector(HalfW, 0, 0));
		Vertices.Add(FVector(HalfW, Depth, Height));
		Vertices.Add(FVector(HalfW, Depth, 0));
		const FVector N(1, 0, 0);
		Normals.Append({N, N, N});
		UVs.Append({FVector2D(0, 0), FVector2D(1, 1), FVector2D(1, 0)});
		Triangles.Append({Base, Base + 1, Base + 2});
	}

	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), bCreateCollision);
	ApplyMaterial(0);
}

void AARPGProceduralMeshActor::GenerateCylinder(float Radius, float Height, int32 Segments)
{
	Segments = FMath::Max(3, Segments);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	const float AngleStep = 2.f * PI / Segments;

	// Side vertices
	for (int32 S = 0; S <= Segments; ++S)
	{
		const float Angle = S * AngleStep;
		const float CosA = FMath::Cos(Angle);
		const float SinA = FMath::Sin(Angle);
		const FVector Normal(CosA, SinA, 0);
		const float U = static_cast<float>(S) / Segments;

		Vertices.Add(FVector(CosA * Radius, SinA * Radius, 0));      // bottom
		Normals.Add(Normal);
		UVs.Add(FVector2D(U, 1));

		Vertices.Add(FVector(CosA * Radius, SinA * Radius, Height)); // top
		Normals.Add(Normal);
		UVs.Add(FVector2D(U, 0));
	}

	// Side triangles
	for (int32 S = 0; S < Segments; ++S)
	{
		const int32 BL = S * 2;
		const int32 TL = BL + 1;
		const int32 BR = BL + 2;
		const int32 TR = BL + 3;
		Triangles.Append({BL, BR, TL, TL, BR, TR});
	}

	// Top cap
	{
		const int32 CenterIdx = Vertices.Num();
		Vertices.Add(FVector(0, 0, Height));
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(0.5f, 0.5f));

		for (int32 S = 0; S < Segments; ++S)
		{
			const float A1 = S * AngleStep;
			const float A2 = (S + 1) * AngleStep;
			const int32 V1 = Vertices.Num();

			Vertices.Add(FVector(FMath::Cos(A1) * Radius, FMath::Sin(A1) * Radius, Height));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(0.5f + FMath::Cos(A1) * 0.5f, 0.5f + FMath::Sin(A1) * 0.5f));

			Vertices.Add(FVector(FMath::Cos(A2) * Radius, FMath::Sin(A2) * Radius, Height));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(0.5f + FMath::Cos(A2) * 0.5f, 0.5f + FMath::Sin(A2) * 0.5f));

			Triangles.Append({CenterIdx, V1, V1 + 1});
		}
	}

	// Bottom cap
	{
		const int32 CenterIdx = Vertices.Num();
		Vertices.Add(FVector(0, 0, 0));
		Normals.Add(-FVector::UpVector);
		UVs.Add(FVector2D(0.5f, 0.5f));

		for (int32 S = 0; S < Segments; ++S)
		{
			const float A1 = S * AngleStep;
			const float A2 = (S + 1) * AngleStep;
			const int32 V1 = Vertices.Num();

			Vertices.Add(FVector(FMath::Cos(A2) * Radius, FMath::Sin(A2) * Radius, 0));
			Normals.Add(-FVector::UpVector);
			UVs.Add(FVector2D(0.5f + FMath::Cos(A2) * 0.5f, 0.5f + FMath::Sin(A2) * 0.5f));

			Vertices.Add(FVector(FMath::Cos(A1) * Radius, FMath::Sin(A1) * Radius, 0));
			Normals.Add(-FVector::UpVector);
			UVs.Add(FVector2D(0.5f + FMath::Cos(A1) * 0.5f, 0.5f + FMath::Sin(A1) * 0.5f));

			Triangles.Append({CenterIdx, V1, V1 + 1});
		}
	}

	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), bCreateCollision);
	ApplyMaterial(0);
}

void AARPGProceduralMeshActor::GenerateCustomMesh(int32 SectionIndex, const TArray<FVector>& Vertices,
	const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UVs,
	const TArray<FColor>& VertexColors)
{
	ProceduralMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, TArray<FProcMeshTangent>(), bCreateCollision);
	ApplyMaterial(SectionIndex);
}

void AARPGProceduralMeshActor::ClearMesh()
{
	ProceduralMesh->ClearAllMeshSections();
}

void AARPGProceduralMeshActor::ApplyMaterial(int32 SectionIndex)
{
	if (MeshMaterial)
	{
		ProceduralMesh->SetMaterial(SectionIndex, MeshMaterial);
	}
}
