void dsp_open(char *dsp_name, int mode, int samp_fmt, int n_chan, int samp_rate);

void dsp_write(void *data, size_t samp_size, size_t n_samps);

void dsp_sync(void);

void dsp_close(void);
