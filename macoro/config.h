

#define MACORO_CPP_20

#ifdef MACORO_CPP_20
#define MACORO_NODISCARD [[nodiscard]]
#define MACORO_FALLTHROUGH [[fallthrough]]
#else
#define MACORO_NODISCARD
#define MACORO_FALLTHROUGH
#endif

