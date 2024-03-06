extends Area2D

# this will determine the camera's speed
var speed = 200


# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	var motion = Vector2()
	
	if Input.is_action_pressed('move_right'):
		motion.x -= 100
	if Input.is_action_pressed('move_left'):
		motion.x += 100
	if Input.is_action_pressed('move_up'):
		motion.y += 100
	if Input.is_action_pressed('move_down'):
		motion.y -= 100
	
	motion = motion.normalized() * speed
	
	position += motion * delta	
	
	pass
