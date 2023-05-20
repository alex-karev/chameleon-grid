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
				var index: int = x * chunk_size.y * chunk_size.z + y * chunk_size.z + z
				if (noise.get_noise_3d(x,y,z)+1)/2 > y/chunk_size.y:
					if z < chunk_size.z/3:
						set_voxel_fast(chunk, index, 2)
					elif z < chunk_size.z/1.5:
						set_voxel_fast(chunk, index, 1)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
