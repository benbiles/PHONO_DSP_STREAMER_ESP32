// DSP runner Ben Biles
/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _DSPRUNNER_H_
#define _DSPRUNNER_H_

#include "esp_err.h"
#include "audio_element.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C" {
#endif


// BEN proto for DSP setup propergate IIR DSP biquad values

// run this in main to change freqncy and Q of LPF

esp_err_t DSP_setup(float freq, float q);



// BEN proto fixed biquads DSP setup
void DSP_setup_fixedBiquad(float b0, float b1, float b2, float a1, float a2);



// BEN creates Audio Element handle

audio_element_handle_t Dsp_init();


#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE


#endif
