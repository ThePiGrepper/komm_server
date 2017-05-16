

//useful constants

#define KOMM_PERROR -1
#define KOMM_PECHO -10
#define komm_checkprotocol(pver) (((pver)==0)||((pver)==2)) //Supports KOMM V2
#define komm_checklen(ptr,size) (*((char*)(ptr)+2) == ((size) - 4))
#define komm_checkcrc(ptr,size) 1 //dummy

//CRC8 generation
char crc8_gen(char *ptr, char size){
  return 0;
}
//Get Device Config
int komm_get_device_config(char *reply){return KOMM_PECHO;}
//Set Device Config
//komm_set_device_config(){return KOMM_PECHO;}
//Get IO Configuration
int komm_get_io_config(char *reply){return KOMM_PECHO;}
//Set IO Configuration
int komm_set_io_config(char *reply,char *config,char size){return KOMM_PECHO;}
//Get Analog Input Values
int komm_get_ain_values(char *reply){return KOMM_PECHO;}
//Get Analog Inputs Status
int komm_get_ain_status(char *reply){return KOMM_PECHO;}
//Set Analog Inputs Thresholds Common
int komm_set_ain_thresholds_common(char *reply,char *data,char size){return KOMM_PECHO;}
//Get Analog Inputs Thresholds Common
int komm_get_ain_thresholds_common(char *reply){return KOMM_PECHO;}
//Set Analog Input Thresholds
int komm_set_ain_thresholds(char *reply,char addr,char *data, char size){return KOMM_PECHO;}
//Get Analog Input Thresholds
int komm_get_ain_thresholds(char *reply,char addr){return KOMM_PECHO;}
//Set Digital Output Values
int komm_set_dout(char *reply,char *data){return KOMM_PECHO;} //data size fixed to 4bytes
//Set Digital Output Value
int komm_set_dout_s(char *reply, char addr,char value){return KOMM_PECHO;}
//Get Digital Output Values
int komm_get_dout(char *reply){return KOMM_PECHO;}
//Get Outputs Clousures
int komm_get_clousure(char *reply){return KOMM_PECHO;}
//Get Output Clousures
int komm_get_clousure_s(char *reply, char addr){return KOMM_PECHO;}
//Get Digital Inputs
int komm_get_din(char *reply){return KOMM_PECHO;}
//Get Outputs Fail
//int komm_get_fail(){return KOMM_PECHO;}
//Reset
int komm_reset(char *reply){return KOMM_PECHO;}
int komm_echo_function(char *reply,char *request){
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
int komm_digest(char *reply,char *ptr,char size){
  //check header
  char rsize;
  if(!komm_checkprotocol(*ptr)) return KOMM_PERROR; //checks for correct protocol version
  if(!komm_checklen(ptr,size)) return KOMM_PERROR; //checks for correct len field.
  if(!komm_checkcrc(ptr,size)) return KOMM_PERROR; //checks CRC correctness
  char cmd = *(ptr+1);
  char len = *(ptr+2);
  char *data_start = ptr + 3;
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
          rsize = komm_set_io_config(reply,data_start,len); break;
        case 0x05:
          rsize = komm_get_ain_values(reply); break;
        case 0x06:
          rsize = komm_get_ain_status(reply); break;
        case 0x07:
          rsize = komm_set_ain_thresholds_common(reply,data_start,len); break;
        case 0x08:
          rsize = komm_get_ain_thresholds_common(reply); break;
        case 0x09:
          rsize = komm_set_ain_thresholds(reply,*(data_start),data_start+1,len-1); break;
        case 0x0A:
          rsize = komm_get_ain_thresholds(reply,*data_start); break;
        case 0x0B:
          rsize = komm_set_dout(reply,data_start); break;
        case 0x0C:
          rsize = komm_set_dout_s(reply,*data_start,*(data_start+1)); break;
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

int main(void)
{
  return 0;
}
