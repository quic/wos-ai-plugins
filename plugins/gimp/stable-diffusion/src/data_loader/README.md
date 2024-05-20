# data-loader

## Usage
```C++
DataLoader loader;

// Setting the TAR filename
loader.load(<tar filename>);

std::vector<int32_t> step_seq;
loader.get_supported_num_steps(step_seq);
// Setting number of steps
loader.set_num_steps(<step value>);

int32_t num_latents = loader.get_num_initial_latents();
const tensor_data_float32_t* latent_ptr = nullptr;
int32_t seed = 0;
loader.get_random_init_latent(<seed idx>, seed, latent_ptr);

const tensor_data_float32_t* uncond_text_embedding_ptr = nullptr;
loader.get_unconditional_text_embedding(uncond_text_embedding_ptr);

int32_t time_step = 0;
loader.get_time_step(<curr step idx>, time_step);
const tensor_data_float32_t* ts_embedding_ptr = nullptr;
loader.get_ts_embedding(<curr step idx>, ts_embedding_ptr);
```

## Test

### Create tar file
Given the pkl files from system team, this will generate the sd_precomute_data.tar file
```python
python3 ./tools/flatten.py --random_latent_init <random-latent-pkl-file> \
                           --time_step_embedding <comma seperated timestep-embedding-pkl-files> \
                           --time_step_list <comma seperated timstep-val-pkl-files> \
                           --unconditional_text_emb <unconditional-text-embedding-pkl-file> \
                           --dumpdir <dump-path>
# Example
'''
python3 ./tools/flatten.py \
        --random_latent_init ./data/app_constant_vectors/random_latent_init/random_init_50_v1.pkl \
        --time_step_embedding ./data/app_constant_vectors/time_step_embedding/time_step_emb_20_v1.pkl,./data/app_constant_vectors/time_step_embedding/time_step_emb_50_v1.pkl \
        --time_step_list ./data/app_constant_vectors/time_step_embedding/time_step_val_scheduler_20_v1.pkl,./data/app_constant_vectors/time_step_embedding/time_step_val_scheduler_50_v1.pkl \
        --unconditional_text_emb ./data/app_constant_vectors/unconditional_text_emb.pkl \
        --dumpdir ./tar_output
'''
# output: ./tar_output/sd_precomute_data.tar
```
This will generate the `sd_precomute_data.tar` file at `<dump-path>`

