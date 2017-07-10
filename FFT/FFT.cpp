/***** FFT.cpp *****/
#include <FFT.h>

void FFT::setup(const char* _name, void (*_callback)(freq_data* inBuffer, freq_data* outBuffer, int size), int _bufferSize, int _priority){
	name = _name;
	bufferSize = _bufferSize;
	priority = _priority;
	callback = _callback;
	
	// allocate ne10 FFT buffers
	timeDomainIn = (ne10_fft_cpx_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_fft_cpx_float32_t));
	timeDomainOut = (ne10_fft_cpx_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_fft_cpx_float32_t));
	frequencyDomainIn = (ne10_fft_cpx_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_fft_cpx_float32_t));
	frequencyDomainOut = (ne10_fft_cpx_float32_t*) NE10_MALLOC (bufferSize * sizeof (ne10_fft_cpx_float32_t));
	cfg = ne10_fft_alloc_c2c_float32_neon (bufferSize);
	
	// create the xenomai task
	int ret = rt_task_create(&(this->task), name, 0, priority, T_JOINABLE);
	if (ret){
		fprintf(stderr, "Unable to create task %s\n", name);
		return;
	}
	
	// create a queue, with prefixed name
	char q_name [30];
	sprintf (q_name, "q_%s", name);
	rt_queue_create(&queue, q_name, AUX_POOL_SIZE, Q_UNLIMITED, Q_PRIO);
	
	// start the task running with the loop function
	rt_task_start(&(this->task), FFT::loop, this);

}

void FFT::loop(void* ptr){
	FFT *instance = (FFT*)ptr;
	rt_print_auto_init(1);
	
	// main task loop
	while(!gShouldStop){
		
		// block on the queue until a message is received
		void* inBuffer;
		ssize_t len = rt_queue_receive(&(instance->queue), &inBuffer, TM_INFINITE);
		if (len != instance->bufferSize*sizeof(float))
			return;
			
		// do the FFT
		instance->execute((float*)inBuffer);

		// call the user's callback
		instance->callback(instance->frequencyDomainIn, instance->frequencyDomainOut, instance->bufferSize);
		
		// invert the FFT and store the result in the pop buffer
		instance->invert(instance->popBufs[!instance->popBuffer]);
		
		rt_queue_free(&(instance->queue), inBuffer);
	}
}

void FFT::execute(float* in){
	for(int i=0; i<bufferSize; i++) {
		timeDomainIn[i].r = in[i];
		timeDomainIn[i].i = 0;
	}
	ne10_fft_c2c_1d_float32_neon (frequencyDomainIn, timeDomainIn, cfg, 0);
}

void FFT::invert(float* out){
	ne10_fft_c2c_1d_float32_neon (timeDomainOut, frequencyDomainOut, cfg, 1);
	for(int i=0; i<bufferSize; i++) {
		out[i] = timeDomainOut[i].r;
	}
}

float FFT::process(float in){
	push(in);
	return pop();
}

void FFT::push(float in){
	if(!pushIndex){
		pushBuf = (float*)rt_queue_alloc(&queue, bufferSize*sizeof(float));
	}
	pushBuf[pushIndex++] = in;
	if (pushIndex >= bufferSize){
		rt_queue_send(&queue, (void*)pushBuf, bufferSize*sizeof(float), Q_NORMAL);
		pushIndex = 0;
	}
}

float FFT::pop(){
	float ret = popBufs[popBuffer][popIndex++];
	if (popIndex >= bufferSize){
		popIndex = 0;
		popBuffer = !popBuffer;
	}
	return ret;
}

void FFT::cleanup(){
	NE10_FREE(timeDomainIn);
	NE10_FREE(timeDomainOut);
	NE10_FREE(frequencyDomainIn);
	NE10_FREE(frequencyDomainOut);
	NE10_FREE(cfg);
}