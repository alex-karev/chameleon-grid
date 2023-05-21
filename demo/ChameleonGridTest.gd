extends ChameleonGrid

@export var grass_material: StandardMaterial3D
@export var wall_material: StandardMaterial3D
# Called when the node enters the scene tree for the first time.
func _ready():
	# Populate databse
	var mat1 = grass_material # Otherwise it crashes. Might be a Godot API bug
	var mat2 = wall_material
	add_material(mat1, Vector2i(1,1))
	add_material(mat2, Vector2i(1,1))
	add_voxel(0, [0,0,0,0,0,0], 1.0, true)
	add_voxel(1, [0,0,0,0,0,0], 0.0, false)
	# Prepare
	position = -chunk_size/2
	randomize()
	# Set noise
	var noise1: FastNoiseLite = FastNoiseLite.new()
	noise1.frequency = 0.025
	noise1.seed = randi()
	var noise2: FastNoiseLite = FastNoiseLite.new()
	noise2.frequency = 0.025
	noise2.seed = randi()
	# Generate chunk
	if count_chunks() == 1:
		remove_chunk(0)
	var chunk = add_chunk(Vector3.ZERO)
	for x in range(1, chunk_size.x-1):
		for y in range(1, chunk_size.y-1):
			for z in range(1, chunk_size.z-1):
				var index: int = z * chunk_size.x * chunk_size.y + y * chunk_size.x + x
				if noise1.get_noise_3d(x,y,z) < 0:
					if noise2.get_noise_3d(x,y,z) < 0:
						set_voxel_fast(chunk, index, 1)
					else:
						set_voxel_fast(chunk, index, 0)
	update_chunk(chunk)
	
func _input(event):
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		_ready()
