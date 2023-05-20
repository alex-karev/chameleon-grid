extends ChameleonGrid


# Called when the node enters the scene tree for the first time.
func _ready():
	# Prepare
	position = -chunk_size/2
	var noise: FastNoiseLite = FastNoiseLite.new()
	noise.frequency = 0.025
	# Set seed
	randomize()
	noise.seed = randi()
	# Generate chunk
	var chunk = add_chunk(Vector3.ZERO)
	for x in range(1, chunk_size.x-1):
		for y in range(1, chunk_size.y-1):
			for z in range(1, chunk_size.z-1):
				var index: int = z * chunk_size.x * chunk_size.y + y * chunk_size.x + x
				if noise.get_noise_3d(x,y,z) < 0:
					set_voxel_fast(chunk, index, 0)
	update_chunk(chunk)
	
func _input(event):
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		_ready()
