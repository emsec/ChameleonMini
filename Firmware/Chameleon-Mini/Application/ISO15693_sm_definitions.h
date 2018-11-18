static enum status{
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

enum request_addressed{
    OUR_ADDR = 1 ,
    NOT_ADDRESSED  = 0 ,
    TO_OTHERS = -1
};


struct ISO15693_parameters {

 uint8_t Flags;
 uint8_t cmd;
 uint8_t *Frame_Uid;
 uint8_t *Frame_params;
 enum request_addressed isaddressed;
 ISO15693UidType tagUid;
 uint16_t Bytes_per_Page  ;

 unsigned int subcarrier_flg:1;
 unsigned int data_rate_flg :1;
 unsigned int inventory_flg :1;
 unsigned int prot_ext_flg  :1;
 unsigned int address_flg   :1;
 unsigned int select_flg    :1;
 unsigned int option_flg    :1;
 unsigned int AFI_flg       :1;
 unsigned int Nb_slot_flg   :1; 
 unsigned int RFU_flg       :1;

};

