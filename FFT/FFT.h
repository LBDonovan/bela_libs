/***** FFT.h *****/
#ifndef __FFT_H_INCLUDED__
#define __FFT_H_INCLUDED__ 

#include <ne10/NE10.h>
#include <Bela.h>
#include <native/task.h>
#include <native/queue.h>

#define AUX_POOL_SIZE 100000
#define AUX_MAX_BUFFER_SIZE AUX_POOL_SIZE/2
#define AUX_DEFAULT_BUFFER_SIZE 1024
#define AUX_DEFAULT_PRIO BELA_AUDIO_PRIORITY-5

typedef ne10_fft_cpx_float32_t freq_data;

class FFT{
	public:
		FFT(){}
		void setup(const char* _name, void (*_callback)(freq_data* inBuffer, freq_data* outBuffer, int size), int _bufferSize, int _priority=AUX_DEFAULT_PRIO);
		void cleanup();
		
		float process(float in);
		
	private:
		const char* name;
		int bufferSize;
		int priority;
		
		RT_TASK task;
		RT_QUEUE queue;
		
		int pushIndex = 0;
		float* pushBuf;
		int popIndex = 0;
		float popBufs[2][AUX_MAX_BUFFER_SIZE];
		int popBuffer = 0;
		void push(float in);
		float pop();
		
		void (*callback)(freq_data* inBuffer, freq_data* outBuffer, int size);
		
		ne10_fft_cpx_float32_t* timeDomainIn;
		ne10_fft_cpx_float32_t* timeDomainOut;
		ne10_fft_cpx_float32_t* frequencyDomainIn;
		ne10_fft_cpx_float32_t* frequencyDomainOut;
		ne10_fft_cfg_float32_t cfg;
		
		void execute(float* in);
		void invert(float* out);
		
		static void loop(void* ptr);
};

#endif