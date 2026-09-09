extends Resource           # Dzięki temu plik jest tylko danymi, a nie Node’em
class_name QuestData       # Pozwala tworzyć zasób z Inspektora

enum Status {              # Nasza własna enumeracja trzech stanów
	INACTIVE,              # 0 – misja jeszcze nie podjęta
	ACTIVE,                # 1 – gracz ją realizuje
	DONE                   # 2 – ukończona
}

@export var id: String                      # Unikalny klucz (np. "deliver_patient")
@export var title: String                   # Krótka nazwa wyświetlana w UI
@export var description: String             # Opis w kilku zdaniach
@export var start_target: String = "quest_giver"   # nazwa grupy node’a startowego
var status: Status = Status.INACTIVE        # Bieżący stan (domyślnie INACTIVE)
@export var objective: String
@export var reward: int = 0       # nagroda w „money”
@export var next_quest_id: String = ""
