extends CharacterBody2D

# Node references
@onready var dialog_popup = get_node("/root/Player/CharacterBody2D/UI/DialogPopup")
@onready var player = get_tree().root.get_node(".")
@onready var animation_sprite = $AnimatedSprite2D

# dialog states
var dialog_state = 0

# npc name
@export var npc_name = ""

const SPEED = 300.0
const JUMP_VELOCITY = -400.0

# Get the gravity from the project settings to be synced with RigidBody nodes.
var gravity = ProjectSettings.get_setting("physics/2d/default_gravity")

# initialize variables
func _ready():
	animation_sprite.play("idle_down")

func _physics_process(delta):

	# Handle jump.
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = JUMP_VELOCITY

	move_and_slide()

#dialog tree    
func dialog(response = ""):
	# Set our NPC's animation to "talk"
	animation_sprite.play("talk_down")   
	# Set dialog_popup npc to be referencing this npc
	dialog_popup.npc = self
	#dialog_popup.npc_name = str(npc_name)   
	# dialog tree
	match dialog_state:
		0:
			# Update dialog tree state
			dialog_state = 1
			# Show dialog popup
			dialog_popup.message = "Howdy Partner. 
									I haven't seen anybody round these parts in quite a while. 
									How's it going these days?"
			dialog_popup.response = "[A] Pretty Good  [B] Bad"
			dialog_popup.open() #re-open to show next dialog
		1:
			match response:
				"A":
					# Update dialog tree state
					dialog_state = 2
					# Show dialog popup
					dialog_popup.message = "Great! Hope you enjoy the days!"
					dialog_popup.response = "[A] Bye"
					dialog_popup.open() #re-open to show next dialog
				"B":
					# Update dialog tree state
					dialog_state = 3
					# Show dialog popup
					dialog_popup.message = "Hope you find a good memory today."
					dialog_popup.response = "[A] Bye"
					dialog_popup.open() #re-open to show next dialog
		2:
			# Update dialog tree state
			dialog_state = 0
			# Close dialog popup
			dialog_popup.close()
			# Set NPC's animation back to "idle"
			animation_sprite.play("idle_down")
		3:
			# Update dialog tree state
			dialog_state = 0
			# Close dialog popup
			dialog_popup.close()
			# Set NPC's animation back to "idle"
			animation_sprite.play("idle_down")
