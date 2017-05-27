// Module for interfacing with GPIOs

// Usage:
// komm.digest(request)

#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "module.h"
#include "user_interface.h"
#include "c_types.h"
#include "c_string.h"

#define PULLUP PLATFORM_GPIO_PULLUP
#define FLOAT PLATFORM_GPIO_FLOAT
#define OUTPUT PLATFORM_GPIO_OUTPUT
#define OPENDRAIN PLATFORM_GPIO_OPENDRAIN
#define INPUT PLATFORM_GPIO_INPUT
#define INTERRUPT PLATFORM_GPIO_INT
#define HIGH PLATFORM_GPIO_HIGH
#define LOW PLATFORM_GPIO_LOW

/*** KOMM core library ***/

//useful constants
#define KOMM_PVERSION 2

#define KOMM_NOFLG_MASK 0x80 //Return type Mask
#define KOMM_FLAGS_MASK 0x60 //Special Flags Mask
#define KOMM_FLAG_RST   0x00 //RESET flag
#define KOMM_FLAG_ERR   0x20 //ERROR flag
#define KOMM_FLAG_ECH   0x40 //ECHO flag
#define KOMM_FLAG_ETC   0x60 //TBD

#define KOMM_PRST (KOMM_NOFLG_MASK | KOMM_FLAG_RST)
#define KOMM_PERROR (KOMM_NOFLG_MASK | KOMM_FLAG_ERR)
#define KOMM_PECHO (KOMM_NOFLG_MASK | KOMM_FLAG_ECH)

#define KOMM_IO_NONE 0x00
#define KOMM_IO_AIN_S 0x01
#define KOMM_IO_AIN_NS 0x02
#define KOMM_IO_DIN 0x03
#define KOMM_IO_OC 0x04
#define KOMM_IO_RLY_5A_NONC 0x05
#define KOMM_IO_RLY_10A_NONC 0x06
#define KOMM_IO_RLY_16A_NONC 0x07
#define KOMM_IO_RLY_20A_NONC 0x08
#define KOMM_IO_RLY_5A_NO 0x09
#define KOMM_IO_RLY_10A_NO 0x0A
#define KOMM_IO_RLY_16A_NO 0x0B
#define KOMM_IO_RLY_20A_NO 0x0C

#define komm_check_special_ret(ret) ((ret) & KOMM_NOFLG_MASK) //check if ret is normal or special
#define komm_check_flag_type(ret) ((ret) & KOMM_FLAGS_MASK) //which special ret
#define komm_checkprotocol(pver) (((pver)==0)||((pver)==2)) //Supports KOMM V2
#define komm_checklen(ptr,size) (*((char*)(ptr)+2) == ((size) - 4))
#define komm_checkcrc(ptr,size) 1 //dummy

//pin starts at 0, based on io_status array order.
#define is_pin_analog(pin) (io_status[pin]>KOMM_IO_NONE && io_status[pin]<=KOMM_IO_AIN_NS)
#define is_pin_din(pin) (io_status[pin] == KOMM_IO_DIN)
#define is_pin_dout(pin) (io_status[pin]>=KOMM_IO_OC && io_status[pin]<=KOMM_IO_RLY_20A_NO)

//CRC8 generation
char crc8_gen(const char *ptr, char size){
  return 0;
}
//Get Device Config
int komm_get_device_config(char *reply){
  *reply = 0x00;
  *(reply+1) = 0x01;
  *(reply+2) = 0x07;
  *(reply+3) = (char)KOMM_PVERSION;
  *(reply+4) = 0x00;
  *(reply+5) = 0x00;
  *(reply+6) = 0x00;
  *(reply+7) = 0x00;
  *(reply+8) = 0x00;
  *(reply+9) = 0x03; //sensus ID
  *(reply+10) = crc8_gen(reply,10);
  return 11;
}

#define KOMM_IOCONFNUM 13
#define KOMM_IONUM 10
#define KOMM_STATE_LEN (KOMM_IONUM + KOMM_IONUM)

char komm_state[KOMM_IONUM+KOMM_IONUM]; //main komm status
char * const io_status = komm_state;
char * const dout_status = (komm_state + KOMM_IONUM);
//analog pins name scheme: analog pin number & 0x80
const char io_map[KOMM_IONUM] = {0x80,0,5,6,7,1,2,8,9,10};
#define KOMM_IOMAP_MASK 0x3F
//Set Device Config
//komm_set_device_config(){return KOMM_PECHO;}
//Get IO Configuration
int komm_get_io_config(char *reply){
  *reply = 0x02;
  *(reply+1) = 0x03;
  *(reply+2) = 0x05;
  *(reply+3) = (io_status[0]<<4) | (io_status[1]&0x0f);
  *(reply+4) = (io_status[2]<<4) | (io_status[3]&0x0f);
  *(reply+5) = (io_status[4]<<4) | (io_status[5]&0x0f);
  *(reply+6) = (io_status[6]<<4) | (io_status[7]&0x0f);
  *(reply+7) = (io_status[8]<<4) | (io_status[9]&0x0f);
  *(reply+8) = crc8_gen(reply,8);
  return 9;
}

//Set IO Configuration
int komm_set_io_config(char *reply,const char *config,char size){
  int i;
  int cend = (KOMM_IONUM > (size<<1))? size<<1 : KOMM_IONUM;
  for(i=0;i<cend;i++)
  {
    char nmode = (i%2)?(*(config+(i>>1))&0x0f):(*(config+(i>>1))>>4);
    //checking valid mode
    if(nmode!=io_status[i] && nmode<KOMM_IOCONFNUM && !(nmode && ((unsigned char)io_map[i]>>7)^(nmode<3))) //it does NOT support mixed io
    {
      //here made physical change to pin
      //very imcompatible with INTERRUPT mode on Lua!!
      switch(nmode)
      {
        case KOMM_IO_NONE:
          if(!(io_map[i]&0x80))
            platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],INPUT,FLOAT);
          //here write equivalent state for analog
          break;
        case KOMM_IO_AIN_S:
        case KOMM_IO_AIN_NS: break;
        case KOMM_IO_DIN:
          platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],INPUT,FLOAT); break;
        case KOMM_IO_OC:
          platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],OPENDRAIN,FLOAT); break;
        case KOMM_IO_RLY_5A_NONC:
        case KOMM_IO_RLY_10A_NONC:
        case KOMM_IO_RLY_16A_NONC:
        case KOMM_IO_RLY_20A_NONC:
        case KOMM_IO_RLY_5A_NO:
        case KOMM_IO_RLY_10A_NO:
        case KOMM_IO_RLY_16A_NO:
        case KOMM_IO_RLY_20A_NO:
          platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],OUTPUT,FLOAT); break;
        default: break;
      }
      //update io_status with new mode
      io_status[i] = nmode;
    }
  }
  //reconstructs reply as a copy of request.(does NOT show actual modified succesful values)
  reply[0]=0x02;
  reply[1]=0x04;
  reply[2]=size;
  for(i=0;i<size;i++) reply[3+i] = config[i];
  reply[3+i] = crc8_gen(reply,3+i);
  return 3+1+i;
}
//Get Analog Input Values
int komm_get_ain_values(char *reply){return KOMM_PECHO;}
//Get Analog Inputs Status
int komm_get_ain_status(char *reply){return KOMM_PECHO;}
//Set Analog Inputs Thresholds Common
int komm_set_ain_thresholds_common(char *reply,const char *data,char size){return KOMM_PECHO;}
//Get Analog Inputs Thresholds Common
int komm_get_ain_thresholds_common(char *reply){return KOMM_PECHO;}
//Set Analog Input Thresholds
int komm_set_ain_thresholds(char *reply,char addr,const char *data, char size){return KOMM_PECHO;}
//Get Analog Input Thresholds
int komm_get_ain_thresholds(char *reply,char addr){return KOMM_PECHO;}
//Set Digital Output Values
int komm_set_dout(char *reply,const char *data, char size){
  int i;
  int cend = (KOMM_IONUM > (size<<3))? size<<3 : KOMM_IONUM;
  int byte_cnt = 3+1+((cend-1)>>3);
  reply[0] = 0x02;
  reply[1] = 0x0B;
  reply[2] = 0x02;
  for(i=3;i<byte_cnt;i++) reply[i]=0; //zeroing data section
  for(i=0;i<cend;i++)
  {
    if(is_pin_dout(i))
    {
      platform_gpio_write(KOMM_IOMAP_MASK & io_map[i],data[i/8]>>(0x7&~i));
      char res = platform_gpio_read(KOMM_IOMAP_MASK & io_map[i]);
      dout_status[i] = res;
      if(res) reply[3+i/8] |= 1<<(0x7&~i);
    }
  }
  reply[byte_cnt] = crc8_gen(reply,byte_cnt);
  return ++byte_cnt;
} //data size fixed to 2bytes
//Set Digital Output Value
int komm_set_dout_s(char *reply, char addr,char value){
  reply[0] = 0x02;
  reply[1] = 0x0C;
  reply[2] = 0x02;
  reply[3] = addr;
  reply[4] = 0;
  if(addr-1 < KOMM_IONUM && is_pin_dout(addr-1))
  {
    platform_gpio_write(KOMM_IOMAP_MASK & io_map[addr-1], value);
    char res = (char) platform_gpio_read(KOMM_IOMAP_MASK & io_map[addr-1]);
    reply[4] = res;
    dout_status[addr-1] = res;
  }
  reply[5] = crc8_gen(reply,5);
  return 6;
}
//Get Digital Output Values
int komm_get_dout(char *reply){
  reply[0] = 0x02;
  reply[1] = 0x0D;
  reply[2] = 0x02;
  reply[3] = ((is_pin_dout(0) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[0]))<< 7) |
             ((is_pin_dout(1) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[1]))<< 6) |
             ((is_pin_dout(2) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[2]))<< 5) |
             ((is_pin_dout(3) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[3]))<< 4) |
             ((is_pin_dout(4) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[4]))<< 3) |
             ((is_pin_dout(5) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[5]))<< 2) |
             ((is_pin_dout(6) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[6]))<< 1) |
             (is_pin_dout(7) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[7]));
  reply[4] = 0x00;
  reply[4] |= ((is_pin_dout(8) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[8]))<< 7) |
              ((is_pin_dout(9) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[9]))<< 6);
  reply[5] = crc8_gen(reply,5);
  return 6;
}

//Get Outputs Clousures
int komm_get_clousure(char *reply){return KOMM_PECHO;}
//Get Output Clousures
int komm_get_clousure_s(char *reply, char addr){return KOMM_PECHO;}
//Get Digital Inputs
int komm_get_din(char *reply){
  reply[0] = 0x02;
  reply[1] = 0x10;
  reply[2] = 0x02;
  reply[3] = ((is_pin_din(0) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[0]))<< 7) |
             ((is_pin_din(1) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[1]))<< 6) |
             ((is_pin_din(2) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[2]))<< 5) |
             ((is_pin_din(3) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[3]))<< 4) |
             ((is_pin_din(4) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[4]))<< 3) |
             ((is_pin_din(5) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[5]))<< 2) |
             ((is_pin_din(6) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[6]))<< 1) |
             (is_pin_din(7) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[7]));
  reply[4] = 0x00;
  reply[4] |= ((is_pin_din(8) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[8]))<< 7) |
              ((is_pin_din(9) && platform_gpio_read(KOMM_IOMAP_MASK & io_map[9]))<< 6);
  reply[5] = crc8_gen(reply,5);
  return 6;
}
//Get Outputs Fail
//int komm_get_fail(){return KOMM_PECHO;}
//Reset
int komm_reset(char *reply){
  reply[0] = 0x02;
  reply[1] = 0x12;
  reply[2] = 0x01;
  reply[3] = 0x00; //add status check maybe??
  reply[4] = crc8_gen(reply,4);
  return KOMM_PRST | 5;
}
int komm_echo_function(char *reply,const char *request){
  return KOMM_PECHO; //return ECHO return value
}

int komm_zero_function(char *reply,char protocol,char cmd,char zero_cnt){
  int i;
  *reply = protocol;
  *(reply+1) = cmd;
  *(reply+2) = zero_cnt;
  for(i = 3;i < (zero_cnt + 3);i++)
  {
    *(reply + i) = 0;
  }
  *(reply+i) = crc8_gen(reply,zero_cnt+3); //assuming crc8 of the whole frame, header included(TBD)
  return i;
}

//parse function
//returns:
//  reply string
//  rsize : reply size
//  mod : returns if the internal state is modified (1) or not (0).
int komm_digest(char *reply,const char *ptr,char size,char *mod){
  //check header
  char rsize;
  if(!komm_checkprotocol(*ptr)) return KOMM_PERROR; //checks for correct protocol version
  if(!komm_checklen(ptr,size)) return KOMM_PERROR; //checks for correct len field.
  if(!komm_checkcrc(ptr,size)) return KOMM_PERROR; //checks CRC correctness
  char cmd = *(ptr+1);
  char len = *(ptr+2);
  const char *data_start = ptr + 3;
  *mod = 0;
  switch(*ptr)
  {
    case 0x00:
      switch(cmd)
      {
        case 0x01:
          rsize = komm_get_device_config(reply); break;
        case 0x02: //set_device_config
          rsize = komm_echo_function(reply,ptr); break;
         default:
          rsize = komm_echo_function(reply,ptr);
      }
      break;
    case 0x02:
      switch(cmd)
      {
        case 0x03:
          rsize = komm_get_io_config(reply); break;
        case 0x04:
          rsize = komm_set_io_config(reply,data_start,len); *mod =1; break;
        case 0x05:
          rsize = komm_get_ain_values(reply); break;
        case 0x06:
          rsize = komm_get_ain_status(reply); break;
        case 0x07:
          rsize = komm_set_ain_thresholds_common(reply,data_start,len); *mod =1; break;
        case 0x08:
          rsize = komm_get_ain_thresholds_common(reply); break;
        case 0x09:
          rsize = komm_set_ain_thresholds(reply,*(data_start),data_start+1,len-1); *mod =1; break;
        case 0x0A:
          rsize = komm_get_ain_thresholds(reply,*data_start); break;
        case 0x0B:
          rsize = komm_set_dout(reply,data_start,len); *mod =1; break;
        case 0x0C:
          rsize = komm_set_dout_s(reply,*data_start,*(data_start+1)); *mod =1; break;
        case 0x0D:
          rsize = komm_get_dout(reply); break;
        case 0x0E:
          rsize = komm_get_clousure(reply);
        case 0x0F:
          rsize = komm_get_clousure_s(reply,*data_start); break;
        case 0x10:
          rsize = komm_get_din(reply); break;
        case 0x11: //get_fail
          rsize = komm_zero_function(reply,2,0x11,4); break;
        case 0x12:
          rsize = komm_reset(reply); break;
        default: //unknown function. return echo.
          rsize = komm_echo_function(reply,ptr);
      }
      break;
    default: rsize = komm_echo_function(reply,ptr);
  }
  //handle return signal
  return rsize;
}

/*** KOMM core library END ***/

// Lua: write( id, string1, [string2], ..., [stringn] )
static int Lkomm_digest( lua_State* L )
{
  char reply[255];
  const char* ptr;
  size_t len,rlen;
  char mod;
  luaL_checktype( L, 1, LUA_TSTRING );
  ptr = lua_tolstring( L, 1, &len );
  rlen = komm_digest(reply,ptr,len,&mod);
  if(komm_check_special_ret(rlen))
  {
    switch(komm_check_flag_type(rlen))
    {
      int i;
      case KOMM_FLAG_RST:
        lua_pushinteger(L,1);
        lua_pushlstring(L, reply,~KOMM_NOFLG_MASK & ~KOMM_FLAGS_MASK & rlen);
        break;
      case KOMM_FLAG_ERR:
        lua_pushinteger(L,2);
        lua_pushlstring(L,reply,0); //dummy(just for test)
        break;
      case KOMM_FLAG_ECH:
        //c_memcpy(reply,ptr,len);
        for(i=0;i<len;i++) *(reply+i)=*(ptr+i);
        lua_pushinteger(L,3);
        lua_pushlstring(L, reply,len);
        break;
      case KOMM_FLAG_ETC:
      default:
        lua_pushinteger(L,2);
        lua_pushlstring(L,reply,0); //dummy(just for test)
    }
  }
  else
  {
    lua_pushinteger(L, 0);
    lua_pushlstring(L, reply,rlen);
  }
  lua_pushinteger(L, mod);
  return 3;
}

static int Lkomm_getState( lua_State* L)
{
  lua_pushlstring(L, komm_state,KOMM_STATE_LEN);
  return 1;
}

static int Lkomm_setState( lua_State* L)
{
  size_t len;
  const char *str;
  int i;
  str = luaL_checklstring(L,-1,&len);
  //update komm state
  //this function does NOT check data validity before uploading it to main state.
  //Very dangerous!!!
  for(i=0;i<len;i++) komm_state[i] = str[i];
  //Update gpio mode and output
  for(i=0;i<KOMM_IONUM;i++)
  {
    switch(komm_state[i])
    {
      case KOMM_IO_NONE:
        if(!(io_map[i]&0x80))
        //  platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],INPUT,FLOAT);
        //here write equivalent state for analog
        break;
      case KOMM_IO_AIN_S:
      case KOMM_IO_AIN_NS: break;
      case KOMM_IO_DIN:
        //platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],INPUT,FLOAT); break;
      case KOMM_IO_OC:
        platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],OPENDRAIN,FLOAT);
        platform_gpio_write(KOMM_IOMAP_MASK & io_map[i],komm_state[i+KOMM_IONUM]);
        komm_state[i+KOMM_IONUM] = platform_gpio_read(KOMM_IOMAP_MASK & io_map[i]);
        break;
      case KOMM_IO_RLY_5A_NONC:
      case KOMM_IO_RLY_10A_NONC:
      case KOMM_IO_RLY_16A_NONC:
      case KOMM_IO_RLY_20A_NONC:
      case KOMM_IO_RLY_5A_NO:
      case KOMM_IO_RLY_10A_NO:
      case KOMM_IO_RLY_16A_NO:
      case KOMM_IO_RLY_20A_NO:
        platform_gpio_mode(KOMM_IOMAP_MASK & io_map[i],OUTPUT,FLOAT);
        platform_gpio_write(KOMM_IOMAP_MASK & io_map[i],komm_state[i+KOMM_IONUM]);
        komm_state[i+KOMM_IONUM] = platform_gpio_read(KOMM_IOMAP_MASK & io_map[i]);
        break;
      default: break;
    }
  }
  return 0;
}

static int Lkomm_info( lua_State* L)
{
    char buf[255];
    c_sprintf(buf, "KOMM Server Version %d", KOMM_PVERSION);
    lua_pushfstring(L, buf);
    return 1;
}

static unsigned i=0;
static int Lkomm_test( lua_State* L)
{
    i=!i;
    platform_gpio_write(2, i);
    lua_pushinteger(L, i);
    return 1;
}

// Module function map, this is how we tell Lua what API our module has
const LUA_REG_TYPE komm_map[] =
{
  { LSTRKEY( "info" ), LFUNCVAL( Lkomm_info ) },
  { LSTRKEY( "digest" ), LFUNCVAL( Lkomm_digest ) },
  { LSTRKEY( "getState" ), LFUNCVAL( Lkomm_getState ) },
  { LSTRKEY( "setState" ), LFUNCVAL( Lkomm_setState ) },
  { LSTRKEY( "test" ), LFUNCVAL( Lkomm_test ) },
  { LNILKEY, LNILVAL } // This map must always end like this
};

NODEMCU_MODULE(KOMM, "komm", komm_map, NULL);
