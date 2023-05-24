extends Node3D

@export var zoom_sensivity: float = 1.2
@export var initial_zoom: float = 50.0
@export var key_control_speed: float = 2.0

var axis: Vector2 = Vector2.ZERO
var mouse_grab: bool = false
var camera: Camera3D

func _input(event):
	axis = Vector2.ZERO
	if event is InputEventMouseButton \
	and event.button_index == MOUSE_BUTTON_MIDDLE:
		mouse_grab = event.pressed
	elif event is InputEventMouseMotion \
	and mouse_grab:
		axis = event.relative

func _process(delta):
	if mouse_grab and axis != Vector2.ZERO:
		rotate_y(-axis.x*delta)
		rotate(basis.x, -axis.y*delta)
	axis = Vector2.ZERO
	if Input.is_key_pressed(KEY_D):
		position += basis.x*delta*30
		#rotate_y(delta * key_control_speed)
	if Input.is_key_pressed(KEY_A):
		position -= basis.x*delta*30
		#rotate_y(-delta * key_control_speed)
	if Input.is_key_pressed(KEY_W):
		position -= basis.z*delta*30
		#rotate(basis.x, -delta * key_control_speed)
	if Input.is_key_pressed(KEY_S):
		position += basis.z*delta*30
		#rotate(basis.x, delta * key_control_speed)
	
