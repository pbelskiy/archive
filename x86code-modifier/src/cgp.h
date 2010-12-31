#if !defined(__CGP_H__)
#define __CGP_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Parse input code to intermediate representation.
 *
 *  @param in_buff An input buffer which must contains x86 code
 *  @param in_size The length of the input code
 *  @param entry_point Entry point of code
 *  @return void
 */
void cgp_init(uint8_t *in_buff, uint32_t in_size, uint32_t entry_point);

/** Free internal resources of CGP.
 *
 *  @return void
 */
void cgp_free(void);

/** Free internal resources of CGP.
 *
 *  @param file_name Name of the output gdl file of current workdir
 *  @return void
 */
void export_to_gdl(const char *file_name);

/** @brief Remove simple obfuscation of input code.
 *
 *  Remove simple obfuscation of input code, to get finished binary you must
 *  use cgp_build(...)
 *
 *  @param file_name Name of the output gdl file of current workdir
 *  @return void
 */
void cgp_remove_simple_obfuscation(void);

/** Build code from intermediate representation.
 *
 *  @param out_buff The pointer to output code.
 *  @return size of buffer
 */
uint32_t cgp_build(uint8_t **out_buff);

/** Build code with adding NOP after every instuction.
 *
 *  @param out_buff The pointer to output code.
 *  @return size of buffer
 */
uint32_t cgp_build_reduntant_nop(uint8_t **out_buff);

/** Build shuffled code linked with JMP instructions.
 *
 *  @param out_buff The pointer to output code.
 *  @return size of buffer
 */
uint32_t cgp_build_spaghetti(uint8_t **out_buff);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
