class_name WorldLayers
extends RefCounted
## Physics categories, independent of render depth. Combat can opt into PEOPLE
## later; ordinary pedestrians never block each other or moving/parked vehicles.
const GEOMETRY := 1
const PEOPLE := 2
const VEHICLES := 4
const PEDESTRIAN_Z := 1.2
