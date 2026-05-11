/*! \file default_configuration.h
	\brief Configuration header

	Default platform-independent configuration
*/

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#ifndef ASSERT
#define ASSERT(x) (void)(x)
#endif //ASSERT

#ifndef BOUNDED
#define BOUNDED(x,y) (void)(x)
#endif //BOUNDED

#endif /* CONFIG_H_ */
