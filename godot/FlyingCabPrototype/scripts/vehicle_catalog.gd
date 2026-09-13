class_name VehicleCatalog
extends Resource
## Stable model IDs for saves; definitions are authored assets, never mutable state.
@export var models: Array[VehicleDefinition] = []

func find_model(id: StringName) -> VehicleDefinition:
	for model in models:
		if model and model.model_id == id:
			return model
	return null

func validation_errors() -> PackedStringArray:
	var errors := PackedStringArray()
	var ids := {}
	for model in models:
		if model == null:
			errors.append("Missing vehicle model")
			continue
		errors.append_array(model.validation_errors())
		if ids.has(model.model_id):
			errors.append("Duplicate vehicle model: " + String(model.model_id))
		ids[model.model_id] = true
	return errors
