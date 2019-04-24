#ifndef FC_PACK_MAX_DEPTH
  // The maximum level of object nesting is around 20% of this value
  #define FC_PACK_MAX_DEPTH 1000
#endif

#ifndef FC_MAX_LOG_OBJECT_DEPTH
   // how many levels of nested objects are displayed in log messages
   #define FC_MAX_LOG_OBJECT_DEPTH 200
#endif

#ifndef FC_MAX_PREALLOC_SIZE
// how many elements will be reserve()d when deserializing vectors
#define FC_MAX_PREALLOC_SIZE (256UL)
#endif
