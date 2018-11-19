/*
 * ISO15693_state_machine.h
 *
 *  Created by rickventura on: Nov-16-2018 
 *        
 *      
 *  A more general 15693 state machine. 
 *  Dereferenced functions (pointer to functions) are used to support tags specific functions.
 *  Reuse of this code is supposed to speedup and generalize code making for different 15693 tags.
 *      
 */


/* Dereferenced Tag specific functions: 
 Function IS015693AppProcess is supposed to be a general 15693 state machine including responses to all possible commands.
 
 Adding a command means: 
 1) defining a response function corresonding to the command;
    When the response function is a standard 15693 response, valid for any 15639 tag:
    the response function shall be defined in this file.   
    When the response function is tag-specific:
      a dereferenced call is needed and a response function shall be defined in a file like mytag.c.
      In this last case a declaration corresponding to the the function is necessary 
      As an example, since a response function for a ISO15693_READSINGLE command is defined in a TITagitstandard.c, 
      the following declaration is added to this file.
      
      extern uint16_t (*readsingle) (uint8_t *FrameBuf, struct ISO15693_parameters *request);
      Notice that (*readsingle) is generic and, in TITagitstandard.c must be assigned a value.     
      In TITagitstandard.c the following lines shall be added:             
      
      uint16_t Tagit_readsingle(uint8_t *FrameBuf, struct ISO15693_parameters *request);   
      uint16_t (*readsingle) (uint8_t *FrameBuf, struct ISO15693_parameters *request) = Tagit_readsingle; 
      
      in this case Tagit_readsingle is the actual function name the one defined in TITagitsandard.c       
      
 2) adding a line in the switch statment corresponding to the ISO15693 command constant along with a call to the response function.
    
	      case ISO15693_CMD_READ_SINGLE:        
	          ResponseByteCount = (*readsingle)(FrameBuf, &request);  derefened call to Tagit_readsingle.       
                  break;         
		  
 
*/
//Dereferenced Tag specific functions 
extern void (*TagGetUid)(ConfigurationUidType Uid) ;
extern void (*TagSetUid)(ConfigurationUidType Uid) ;
extern uint16_t (*readsingle) (uint8_t *FrameBuf, struct ISO15693_parameters *request);

// ISO15693 functions
uint16_t ISO15693_stay_quiet(enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request) ;
uint16_t ISO15693_select(enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request);
uint16_t ISO15693_reset_to_ready(enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request);
uint16_t ISO15693_writesingle(uint8_t *FrameBuf, struct ISO15693_parameters *request);

// pointers to functions included in the standard ISO15693 machine
uint16_t (*stay_quiet)(enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request) =  ISO15693_stay_quiet;
uint16_t (*select)(enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request) = ISO15693_select;
uint16_t (*reset_to_ready) (enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request) = ISO15693_reset_to_ready;
uint16_t (*writesingle)(uint8_t *FrameBuf, struct ISO15693_parameters *request) = ISO15693_writesingle;


             


struct ISO15693_parameters ISO15693_extract_par (uint8_t *FrameBuf)
{
  /* ISO 15693 general request: 
    [FLAGS(1BYTE)][CMD(1BYTE)][OPTIONAL UID(8BYTES)][DATA SIZE AND MEANING DEPENDS ON CMD]
                               ^ Frame_Uid          ^ Frame_params
  */
  struct ISO15693_parameters request;
  
  
  request.Flags = FrameBuf[ISO15693_ADDR_FLAGS];     // FrameBuf[0x00]
  request.cmd   = FrameBuf[ISO15693_ADDR_FLAGS + 1]; // FrameBuf[0x01] 
  request.Frame_Uid =(uint8_t *) NULL;
  request.Frame_params = (uint8_t *)NULL;
  request.Bytes_per_Page = TAG_BYTES_PER_PAGE;

 /* set flags according to 15693-3 2009_A2_2015.pdf manual Tables 3,4,5 page 9 */
  request.subcarrier_flg= FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_SUBCARRIER ? 1 : 0;
  request.data_rate_flg = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_DATARATE   ? 1 : 0;
  request.inventory_flg = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_INVENTORY  ? 1 : 0;
  request.prot_ext_flg  = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_PROT_EXT   ? 1 : 0;
  request.address_flg   = 0;
  request.select_flg    = 0;
  request.option_flg    = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION     ? 1 : 0;
  request.RFU_flg       = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_RFU        ? 1 : 0;
  request.AFI_flg      = 0;
  request.Nb_slot_flg   = 0;
 
   if (request.inventory_flg ) { // when inventory flag is set flags set according to table 5
   
    request.AFI_flg       = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_AFI ?      1 : 0;
    request.Nb_slot_flg   = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_NB_SLOTS ? 1 : 0;    

  } else { // sets according to table 4
    request.address_flg  = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_ADDRESS   ? 1 : 0;
    request.select_flg   = FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_SELECT    ? 1 : 0;    
  }

  (*TagGetUid)(request.tagUid); // get the actual tag uid using a pointer to a tag specific function whatever it maight be

  if (request.address_flg){ // addressed request       
    
    request.Frame_Uid = &FrameBuf[ISO15693_ADDR_FLAGS + 2]; // set pointer to request Uid    
    
    if (ISO15693CompareUid(request.Frame_Uid, request.tagUid)) {            
     //request addressed to this tag
      request.isaddressed = OUR_ADDR; /*request is addressed to this tag.  isaddressed tells more tan the related address_flag: */
      request.Frame_params = request.Frame_Uid + ActiveConfiguration.UidSize; // Frame_params pointer set to the first parameter in the request   	    
     }
     else   
       request.isaddressed = TO_OTHERS; // request addressed to other tags            
      
  }
  else { // request not addressed
      request.isaddressed  = NOT_ADDRESSED; /* request not addressed*/ 
      request.Frame_params = &FrameBuf[ISO15693_ADDR_FLAGS + 2];   	        
  }
  
 return request;   
 
}



uint16_t  ISO15693_inventory ( uint8_t *FrameBuf , struct ISO15693_parameters *request )
{

   uint16_t ResponseByteCount = 0;

   if (request->inventory_flg) {//The Inventory_flag shall be set to 1 according to 15693-3 2009_A2_2015.pdf manual 10.3.1
   	FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
   	FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_INVENTORY_DSFID;
   	ISO15693CopyUid(&FrameBuf[ISO15693_RES_ADDR_PARAM + 0x01], request->tagUid);
   	ResponseByteCount = 10;
   } else ResponseByteCount = 0; //If the VICC detects an error, it shall remain silent 15693-3 2009_A2_2015.pdf manual 10.3.1.
     
   return ResponseByteCount;
  
}



uint16_t ISO15693_writesingle(uint8_t *FrameBuf , struct ISO15693_parameters *request)
{ // returns responsByteCount
 
  uint16_t  ResponseByteCount = 0;
  uint16_t PageAddress , MemLocation ;

 
   /* ISO 15693 READ SINGLE request: 
      addressed    [FLAGS(1BYTE)][CMD(1BYTE)][UID(8BYTES)][BLOCK #][4 bytes(to be written)]
                                                          ^  
                                                         request->Frame_params

       NOT_ADDRESSED  [FLAGS(1BYTE)][CMD(1BYTE)][BLOCK #][4 bytes(to be written)]
                                                ^  
                                               request->Frame_params
   */

  // request->Frame_params is assumed to be correctly assigned by the extract_param function to point to the block # parameter(PageAddress) 
  PageAddress = *request->Frame_params++; // Assign PageAddres and make request->Frame_params to point to the data to be written  

  MemLocation = PageAddress * request->Bytes_per_Page;
  MemoryWriteBlock( request->Frame_params, MemLocation, request->Bytes_per_Page);
  FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
  ResponseByteCount = 1;
 

 return ResponseByteCount;  
}

uint16_t ISO15693_reset_to_ready(enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request){

  uint16_t ResponseByteCount = 0;
   
   if (request->address_flg){
        if ( request->isaddressed == OUR_ADDR) {
          FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
          ResponseByteCount = 1;
          State = STATE_READY;
        } else ResponseByteCount = 0; // not addressed to us

   } else if ( *State == STATE_SELECTED && request->select_flg) {
        // 15693-3 2009_A2_2015.pdf manual 10.3.2 page 13 state picture
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount = 1;
        State = STATE_READY;
   }
  return ResponseByteCount ;  
}

uint16_t ISO15693_stay_quiet(enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request)
{
 /*15693-3 2009_A2_2015.pdf manual 10.3.2
   When receiving the Stay quiet command, the VICC shall enter the quiet state and shall NOT send back a
   response. There is NO response to the Stay quiet command*/
 uint16_t  ResponseByteCount = 0;

 // The stay quiet command command  shall  always be be executed executed in in Addressed Addressed mode

 if (request->isaddressed == OUR_ADDR)
    *State = STATE_QUIET;
 else if (*State == STATE_SELECTED)
 	 ResponseByteCount = 0;

 return ResponseByteCount;

}

uint16_t  ISO15693_select (enum status *State , uint8_t *FrameBuf, struct ISO15693_parameters *request)
{
 //15693-3 2009_A2_2015.pdf manual 10.4.6:
  uint16_t  ResponseByteCount = 0;

  
  if (request->isaddressed == OUR_ADDR && !request->select_flg) {// for a select command the Select_flag is set to 0. The Address_flag is set to 1.
    //if the UID is equal to its own UID, the VICC shall enter the selected state and shall send a response
       *State = STATE_SELECTED;                     
       FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
       ResponseByteCount = 1;         
    	
  } else if (request->isaddressed == TO_OTHERS ){
     if ( *State == STATE_SELECTED ){ 
	     /* if in selected state and different UID it shall return to the Ready state and 
	        shall not send a response.15693-3 2009_A2_2015.pdf manual 10.4.6: */
  	  *State = STATE_READY;
           ResponseByteCount = 0;


     } else 
	//not in select state and UID different from its own, the VICC shall remain in its state and shall not return a response.
        ResponseByteCount = 0;

  } else { // not addressed 
    /* The Select command shall always be executed in Addressed mode */
    ResponseByteCount = 0;
  }

  return ResponseByteCount;

}
        

uint8_t ISO15693_status_check ( enum status *State , struct ISO15693_parameters *request , uint16_t *ResponseByteCount )
                               
{
 uint8_t mayExecute = 1 ;

 switch ( *State ){

  case  STATE_READY:
      if (request->select_flg) 
           mayExecute = 0;
      break;

  case STATE_QUIET:

      if (!request->address_flg || 
          request->inventory_flg) 
          mayExecute = 0;         

    break;   

  case  STATE_SELECTED:

     if (request->select_flg){
	mayExecute = 1;
    
     } else if(request->cmd == ISO15693_CMD_SELECT && request->isaddressed == TO_OTHERS){
	/* 
 	   if the UID is different to its own and in selected state, the VICC shall return to the Ready state and
	   shall not send a response. The Select command shall always be executed in Addressed mode. (The
	   Select_flag is set to 0. The Address_flag is set to 1. see ISO_IEC_15693-3-2009_A2_2015.pdf page 29	)
	*/
          *State = STATE_READY;
          *ResponseByteCount = 0;    
           mayExecute = 0;    
     }   
     break;

 
  
 }//end switch
 

 return mayExecute; // returns whether a command may be executed

}
uint16_t IS015693AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{   
     
   
    if (FrameBytes >= ISO15693_MIN_FRAME_SIZE) {
        if(ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE)) {
         
            // At this point, we have a valid ISO15693 frame            
            uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;            
            uint8_t mayExecute = 0;
	    struct ISO15693_parameters request;            
            
            
            ResponseByteCount = 0;
            request    = ISO15693_extract_par (FrameBuf);	
            mayExecute = ISO15693_status_check( &State , &request , &ResponseByteCount );

            if (mayExecute){ 
	      switch ( request.cmd ) {

	        case ISO15693_CMD_STAY_QUIET:         
                  ResponseByteCount = ISO15693_stay_quiet(&State , FrameBuf, &request);
                  break;

                case ISO15693_CMD_SELECT:             
                  ResponseByteCount = ISO15693_select (&State , FrameBuf, &request);
                  break;

	        case ISO15693_CMD_RESET_TO_READY:     
                  ResponseByteCount = ISO15693_reset_to_ready(&State , FrameBuf, &request); 
                  break;

                case ISO15693_CMD_INVENTORY:  
		  ResponseByteCount = ISO15693_inventory(FrameBuf , &request);         
                  break;       

   	       case ISO15693_CMD_WRITE_SINGLE:       
	          ResponseByteCount = ISO15693_writesingle(FrameBuf, &request);         
	          break;

	      case ISO15693_CMD_READ_SINGLE:        
	          ResponseByteCount = (*readsingle)(FrameBuf, &request);         
                  break;
              default:
                ResponseByteCount = 0;
                break;
            }// end switch

           } 

           if (ResponseByteCount > 0) {
                /* There is data to be sent. Append CRC */
                ISO15693AppendCRC(FrameBuf, ResponseByteCount);
                ResponseByteCount += ISO15693_CRC16_SIZE;
            }

            return ResponseByteCount;

        } else { // Invalid CRC
            return ISO15693_APP_NO_RESPONSE;
        }
    } else { // Min frame size not met
        return ISO15693_APP_NO_RESPONSE;
    }

}


