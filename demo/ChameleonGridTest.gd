extends ChameleonGrid

@export var grass_material: StandardMaterial3D
@export var wall_material: StandardMaterial3D
@export var dirt_material: StandardMaterial3D

var noise1: FastNoiseLite
var noise2: FastNoiseLite
var noise3: FastNoiseLite

# Called when the node enters the scene tree for the first time.
func _ready():
	# Populate databse
	var mat1 = grass_material # Otherwise it crashes. Might be a Godot API bug
	var mat2 = wall_material
	var mat3 = dirt_material
	add_material(mat1, Vector2i(1,1))
	add_material(mat2, Vector2i(1,1))
	add_material(mat3, Vector2i(1,1))
	add_voxel(0, [0,0,0,0,0,0], 1.0, true)
	add_voxel(1, [0,0,0,0,0,0], 0.0, false)
	add_voxel(2, [0,0,0,0,0,0], 0.5, true)
	# Prepare
	position = -chunk_size
	randomize()
	# Set noise
	noise1 = FastNoiseLite.new()
	noise1.frequency = 0.025
	noise2 = FastNoiseLite.new()
	noise2.frequency = 0.025
	noise3 = FastNoiseLite.new()
	noise3.frequency = 0.025
	generate()

func generate():
	noise1.seed = randi()
	noise2.seed = randi()
	noise3.seed = randi()
	# Generate chunk
	if count_chunks() != 0:
		remove_chunk(0)
		remove_chunk(1)
		remove_chunk(2)
		remove_chunk(3)
	generate_chunk(Vector3.ZERO)
	generate_chunk(Vector3i(1,0,0))
	generate_chunk(Vector3i(0,0,1))
	generate_chunk(Vector3i(1,0,1))

func generate_chunk(chunk_index: Vector3i):
	var chunk = add_chunk(chunk_index)
	for x in chunk_size.x:
		for y in chunk_size.y:
			for z in chunk_size.z:
				var index: int = z * chunk_size.x * chunk_size.y + y * chunk_size.x + x
				var pos = Vector3(x,y,z) + Vector3(chunk_size-Vector3i.ONE*2)*Vector3(chunk_index)
				if (noise1.get_noise_3dv(pos)+1)/2*y/chunk_size.y < 0.2:
					if noise2.get_noise_3dv(pos) < 0:
						if noise3.get_noise_3dv(pos) < 0:
							set_voxel_fast(chunk, index, 2)
						else:
							set_voxel_fast(chunk, index, 1)
					else:
						set_voxel_fast(chunk, index, 0)
	update_chunk(chunk)
	
func _input(event):
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		generate()

