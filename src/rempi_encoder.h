#ifndef __REMPI_ENCODER_H_
#define __REMPI_ENCODER_H_

#include "rempi_event.h"

using namespace std;

class rempi_encoder
{
  public:
    rempi_encoder();
    virtual void encode(rempi_event *events, size_t length, char *serialized, size_t *size);
    virtual void decode(char *serialized, rempi_event *events, size_t *length);
};

class rempi_encoder_no_encoding : public rempi_encoder
{
  public:
    virtual void encode(rempi_event *events, size_t length, char *serialized, size_t *size);
    virtual void decode(char *serialized, rempi_event *events, size_t *length);
    rempi_encoder_no_encoding();
};

#endif /* __REMPI_ENCODER_H__ */
