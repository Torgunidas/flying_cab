#include "FlyingCabLivingWorldProfile.h"
#include "FlyingCabCityData.h"

TArray<FFlyingCabLivingRouteDefinition> UFlyingCabLivingWorldProfile::BuildCityRoutes()
{
	TArray<FFlyingCabLivingRouteDefinition> Routes;
	const FVector2D Min = FlyingCabCityData::GetMinimapWorldMin();
	const FVector2D Max = FlyingCabCityData::GetMinimapWorldMax();
	const FVector2D Mid = (Min + Max) * 0.5;
	const double L = Min.X + 1500, R = Max.X - 1500, B = Min.Y + 1500, T = Max.Y - 1500;
	const double E = Mid.Y - 250, W = Mid.Y + 250, N = Mid.X + 250, S = Mid.X - 250;
	auto Node = [](double X, double Z, float Limit = 1400.f,
		EFlyingCabLivingRouteAction Action = EFlyingCabLivingRouteAction::PassThrough,
		FName Stop = NAME_None, float Wait = 0.f, FName Signal = NAME_None)
	{
		FFlyingCabLivingRouteNode Result;
		Result.LocalLocation = FVector(X, 0, Z);
		Result.SpeedLimit = Limit; Result.Action = Action; Result.StopId = Stop;
		Result.WaitDuration = Wait; Result.TrafficSignal = Signal;
		return Result;
	};
	auto Add = [&](FName Id, int32 Count, TArray<FFlyingCabLivingRouteNode> Nodes)
	{
		FFlyingCabLivingRouteDefinition Route;
		Route.RouteId = Id; Route.RouteClass = EFlyingCabLivingRouteClass::Express;
		Route.CruiseSpeed = 1400.f; Route.Acceleration = 420.f; Route.Deceleration = 950.f;
		Route.MinimumSpacing = 480.f; Route.VehicleCornerSmoothingDistance = 650.f;
		Route.SpawnCount = Count; Route.Nodes = MoveTemp(Nodes);
		Route.VehicleColors = {FLinearColor(.05f,.8f,1.f), FLinearColor(1.f,.3f,.05f),
			FLinearColor(.9f,.08f,.6f), FLinearColor(.2f,.9f,.35f)};
		return Routes.Add(MoveTemp(Route));
	};
	auto HGate = [&](double X, double Z) { return Node(X,Z,1400.f,EFlyingCabLivingRouteAction::PassThrough,NAME_None,0,TEXT("Horizontal")); };
	auto VGate = [&](double X, double Z) { return Node(X,Z,1400.f,EFlyingCabLivingRouteAction::PassThrough,NAME_None,0,TEXT("Vertical")); };
	Add(TEXT("Metro.Ring.Clockwise"), 8, {Node(L,B),Node(L,T),Node(R,T),Node(R,B)});
	Add(TEXT("Metro.Ring.CounterClockwise"), 8,
		{Node(L-500,B-500),Node(R+500,B-500),Node(R+500,T+500),Node(L-500,T+500)});
	Add(TEXT("Metro.Cross.East"), 3, {Node(L+1000,E),HGate(Mid.X-1400,E),Node(Mid.X+1400,E),
		Node(R-1000,E),Node(R,E-850),Node(R,B),Node(L,B),Node(L,E-850)});
	Add(TEXT("Metro.Cross.West"), 3, {Node(R-1000,W),HGate(Mid.X+1400,W),Node(Mid.X-1400,W),
		Node(L+1000,W),Node(L,W+850),Node(L,T),Node(R,T),Node(R,W+850)});
	Add(TEXT("Metro.Cross.North"), 3, {Node(N,B+300),VGate(N,Mid.Y-1500),Node(N,Mid.Y+1500),
		Node(N,T-900),Node(N+950,T),Node(R,T),Node(R,B),Node(N+950,B)});
	Add(TEXT("Metro.Cross.South"), 3, {Node(S,T-300),VGate(S,Mid.Y+1500),Node(S,Mid.Y-1500),
		Node(S,B+900),Node(S-950,B),Node(L,B),Node(L,T),Node(S-950,T)});

	for (const FFlyingCabNeighborhoodDefinition& Estate : FlyingCabCityData::GetNeighborhoods())
	{
		const double X = Estate.Center.X, Z = Estate.Center.Z;
		const FName A(*(Estate.NeighborhoodId.ToString() + TEXT(".TransitA")));
		const FName D(*(Estate.NeighborhoodId.ToString() + TEXT(".TransitB")));
		TArray<FFlyingCabLivingRouteNode> Path = {
			Node(X-1200,Z-100,450,EFlyingCabLivingRouteAction::Land,A,5),
			Node(X-600,Z+250,550,EFlyingCabLivingRouteAction::TakeOff),
			Node(X+500,Z+250,600),Node(X+1200,Z-100,450,EFlyingCabLivingRouteAction::Land,D,4),
			Node(X+2500,Z+300,550,EFlyingCabLivingRouteAction::TakeOff)};
		if (Z > Mid.Y)
		{
			Path.Append({Node(X+2500,Z-700,650),Node(X+2500,W+850,800),Node(X+1900,W,1000)});
			if (X > Mid.X) Path.Append({HGate(Mid.X+1400,W),Node(Mid.X-1400,W)});
			Path.Append({Node(L+1000,W),Node(L,W+850),Node(L,T),Node(X-2700,T),
				Node(X-2700,Z+400,800)});
		}
		else
		{
			Path.Append({Node(X+2500,Z+700,650),Node(X+2500,E-850,800),Node(X+3000,E,1000)});
			if (X < Mid.X) Path.Append({HGate(Mid.X-1400,E),Node(Mid.X+1400,E)});
			Path.Append({Node(R-1000,E),Node(R,E-850),Node(R,B),Node(X-2700,B),
				Node(X-2700,Z-700,800),Node(X-2700,Z+250,600)});
		}
		const int32 Index = Add(FName(*(TEXT("Metro.Commuter.") + Estate.NeighborhoodId.ToString())),3,MoveTemp(Path));
		Routes[Index].RouteClass = EFlyingCabLivingRouteClass::LandingApproach;
		Routes[Index].VehicleColors = {Estate.Color, Estate.Color * .65f, FLinearColor(.7f,.7f,.65f)};
		// Ambient platforms are separate from player taxi pickup/dropoff curbs.
		FFlyingCabLivingRouteDefinition Ped;
		Ped.RouteId = FName(*(TEXT("Metro.Pedestrians.") + Estate.NeighborhoodId.ToString()));
		Ped.AgentKind = EFlyingCabLivingAgentKind::Pedestrian;
		Ped.RouteClass = EFlyingCabLivingRouteClass::Pedestrian;
		Ped.CruiseSpeed = 145; Ped.Acceleration = 300; Ped.Deceleration = 500;
		Ped.MinimumSpacing = 90; Ped.SpawnCount = 2;
		Ped.Nodes = {Node(X-1600,Z-170,0,EFlyingCabLivingRouteAction::ExitBuilding,NAME_None,.5f),
			Node(X-1400,Z-170,0,EFlyingCabLivingRouteAction::BoardVehicle,A),
			Node(X+1400,Z-170,0,EFlyingCabLivingRouteAction::ExitVehicle,D),
			Node(X+1750,Z-170,0,EFlyingCabLivingRouteAction::EnterBuilding,NAME_None,6),
			Node(X+1750,Z-170,0,EFlyingCabLivingRouteAction::ExitBuilding,NAME_None,.5f),
			Node(X+1400,Z-170,0,EFlyingCabLivingRouteAction::BoardVehicle,D),
			Node(X-1400,Z-170,0,EFlyingCabLivingRouteAction::ExitVehicle,A),
			Node(X-1600,Z-170,0,EFlyingCabLivingRouteAction::EnterBuilding,NAME_None,5)};
		// Separate arrival/departure strips across the terrace depth. Opposite walkers
		// must not meet head-on on a single spline and block the next shuttle forever.
		for (int32 I=0; I<Ped.Nodes.Num(); ++I)
			Ped.Nodes[I].LocalLocation.Y = (I<2 || (I>=4 && I<6)) ? -75.f : 75.f;
		Routes.Add(MoveTemp(Ped));
	}
	return Routes;
}
