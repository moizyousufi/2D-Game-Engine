extends CanvasLayer

# Node refs
@onready var animation_player = $"/root/Player/CharacterBody2D/AnimationPlayer"

#gets the values of our npc from our NPC scene and sets it in the label values
var npc_name : set = npc_name_set
var message: set = message_set
var response: set = response_set

# reference to NPC
var npc

#sets the npc name with the value received from NPC
func npc_name_set(new_value):
	npc_name = new_value
	$/root/Player/CharacterBody2D/UI/DialogPopup/Dialog/NPC.text = new_value

#sets the message with the value received from NPC
func message_set(new_value):
	message = new_value
	$/root/Player/CharacterBody2D/UI/DialogPopup/Dialog/Message.text = new_value

#sets the response with the value received from NPC
func response_set(new_value):
	response = new_value
	$/root/Player/CharacterBody2D/UI/DialogPopup/Dialog/Response.text = new_value

# ------------------- Processing ---------------------------------
# no input on hidden
func _ready():
	npc = get_node("../../../SpawnedNPC/NPC")
	set_process_input(true)
	print(npc)

#opens the dialog
func open():
	get_tree().paused = true
	self.visible = true
	animation_player.play("typewriter")

#closes the dialog  
func close():
	get_tree().paused = false
	self.visible = false

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass

func _on_animation_player_animation_finished(anim_name):
	set_process(true)

# ------------------- Dialog -------------------------------------
func _input(event):
	if event is InputEventKey:
		if event.is_action_pressed('ui_interact'):
			if event.keycode == KEY_A:  
				npc.dialog("A")
			elif event.is_action_pressed('KEY_B'):
				npc.dialog("B")
