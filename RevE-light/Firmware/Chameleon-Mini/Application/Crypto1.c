#include "Crypto1.h"

#define PRNG_MASK        0x002D0000UL
/* x^16 + x^14 + x^13 + x^11 + 1 */

#define PRNG_SIZE        4 /* Bytes */

#define LFSR_MASK_EVEN    0x2010E1UL
#define LFSR_MASK_ODD    0x3A7394UL
/* x^48 + x^43 + x^39 + x^38 + x^36 + x^34 + x^33 + x^31 + x^29 +
 * x^24 + x^23 + x^21 + x^19 + x^13 + x^9 + x^7 + x^6 + x^5 + 1 */

#define LFSR_SIZE        6 /* Bytes */

/* Functions fa, fb and fc in filter output network. Definitions taken
 * from Timo Kasper's thesis */
#define FA(x3, x2, x1, x0) ( \
    ( (x0 | x1) ^ (x0 & x3) ) ^ ( x2 & ( (x0 ^ x1) | x3 ) ) \
)

#define FB(x3, x2, x1, x0) ( \
    ( (x0 & x1) | x2 ) ^ ( (x0 ^ x1) & (x2 | x3) ) \
)

#define FC(x4, x3, x2, x1, x0) ( \
    ( x0 | ( (x1 | x4) & (x3 ^ x4) ) ) ^ ( ( x0 ^ (x1 & x3) ) & ( (x2 ^ x3) | (x1 & x4) ) ) \
)

/* Create tables from function fa, fb and fc for faster access */
static const uint8_t TableAB[5][16] = {
    { /* fa with Input {3,2,1,0} = (0,0,0,0) to (1,1,1,1) shifted by 0 */
        FA(0,0,0,0) << 0, FA(0,0,0,1) << 0, FA(0,0,1,0) << 0, FA(0,0,1,1) << 0,
        FA(0,1,0,0) << 0, FA(0,1,0,1) << 0, FA(0,1,1,0) << 0, FA(0,1,1,1) << 0,
        FA(1,0,0,0) << 0, FA(1,0,0,1) << 0, FA(1,0,1,0) << 0, FA(1,0,1,1) << 0,
        FA(1,1,0,0) << 0, FA(1,1,0,1) << 0, FA(1,1,1,0) << 0, FA(1,1,1,1) << 0,
    },
    { /* fb with Input {3,2,1,0} = (0,0,0,0) to (1,1,1,1) shifted by 1 */
        FB(0,0,0,0) << 1, FB(0,0,0,1) << 1, FB(0,0,1,0) << 1, FB(0,0,1,1) << 1,
        FB(0,1,0,0) << 1, FB(0,1,0,1) << 1, FB(0,1,1,0) << 1, FB(0,1,1,1) << 1,
        FB(1,0,0,0) << 1, FB(1,0,0,1) << 1, FB(1,0,1,0) << 1, FB(1,0,1,1) << 1,
        FB(1,1,0,0) << 1, FB(1,1,0,1) << 1, FB(1,1,1,0) << 1, FB(1,1,1,1) << 1,
    },
    { /* fb with Input {3,2,1,0} = (0,0,0,0) to (1,1,1,1) shifted by 2 */
        FB(0,0,0,0) << 2, FB(0,0,0,1) << 2, FB(0,0,1,0) << 2, FB(0,0,1,1) << 2,
        FB(0,1,0,0) << 2, FB(0,1,0,1) << 2, FB(0,1,1,0) << 2, FB(0,1,1,1) << 2,
        FB(1,0,0,0) << 2, FB(1,0,0,1) << 2, FB(1,0,1,0) << 2, FB(1,0,1,1) << 2,
        FB(1,1,0,0) << 2, FB(1,1,0,1) << 2, FB(1,1,1,0) << 2, FB(1,1,1,1) << 2,
    },
    { /* fa with Input {3,2,1,0} = (0,0,0,0) to (1,1,1,1) shifted by 3 */
        FA(0,0,0,0) << 3, FA(0,0,0,1) << 3, FA(0,0,1,0) << 3, FA(0,0,1,1) << 3,
        FA(0,1,0,0) << 3, FA(0,1,0,1) << 3, FA(0,1,1,0) << 3, FA(0,1,1,1) << 3,
        FA(1,0,0,0) << 3, FA(1,0,0,1) << 3, FA(1,0,1,0) << 3, FA(1,0,1,1) << 3,
        FA(1,1,0,0) << 3, FA(1,1,0,1) << 3, FA(1,1,1,0) << 3, FA(1,1,1,1) << 3,
    },
    { /* fb with Input {3,2,1,0} = (0,0,0,0) to (1,1,1,1) shifted by 4 */
        FB(0,0,0,0) << 4, FB(0,0,0,1) << 4, FB(0,0,1,0) << 4, FB(0,0,1,1) << 4,
        FB(0,1,0,0) << 4, FB(0,1,0,1) << 4, FB(0,1,1,0) << 4, FB(0,1,1,1) << 4,
        FB(1,0,0,0) << 4, FB(1,0,0,1) << 4, FB(1,0,1,0) << 4, FB(1,0,1,1) << 4,
        FB(1,1,0,0) << 4, FB(1,1,0,1) << 4, FB(1,1,1,0) << 4, FB(1,1,1,1) << 4,
    }
};

static const uint8_t TableC[32] = {
    /* fc with Input {4,3,2,1,0} = (0,0,0,0,0) to (1,1,1,1,1) */
    FC(0,0,0,0,0), FC(0,0,0,0,1), FC(0,0,0,1,0), FC(0,0,0,1,1),
    FC(0,0,1,0,0), FC(0,0,1,0,1), FC(0,0,1,1,0), FC(0,0,1,1,1),
    FC(0,1,0,0,0), FC(0,1,0,0,1), FC(0,1,0,1,0), FC(0,1,0,1,1),
    FC(0,1,1,0,0), FC(0,1,1,0,1), FC(0,1,1,1,0), FC(0,1,1,1,1),
    FC(1,0,0,0,0), FC(1,0,0,0,1), FC(1,0,0,1,0), FC(1,0,0,1,1),
    FC(1,0,1,0,0), FC(1,0,1,0,1), FC(1,0,1,1,0), FC(1,0,1,1,1),
    FC(1,1,0,0,0), FC(1,1,0,0,1), FC(1,1,0,1,0), FC(1,1,0,1,1),
    FC(1,1,1,0,0), FC(1,1,1,0,1), FC(1,1,1,1,0), FC(1,1,1,1,1),
};

/* Split Crypto1 state into even and odd bits to speed up the output filter network */
static uint8_t StateEven[LFSR_SIZE/2] = {0};
static uint8_t StateOdd[LFSR_SIZE/2] = {0};

/* Proceed LFSR by one clock cycle */
static void Crypto1LFSR(uint8_t In) {
    uint8_t Feedback = 0;

    /* Calculate feedback according to LFSR taps. XOR all 6 state bytes
    * into a single bit. */
    Feedback ^= StateEven[0] & (uint8_t) (LFSR_MASK_EVEN >> 0);
    Feedback ^= StateEven[1] & (uint8_t) (LFSR_MASK_EVEN >> 8);
    Feedback ^= StateEven[2] & (uint8_t) (LFSR_MASK_EVEN >> 16);

    Feedback ^= StateOdd[0] & (uint8_t) (LFSR_MASK_ODD >> 0);
    Feedback ^= StateOdd[1] & (uint8_t) (LFSR_MASK_ODD >> 8);
    Feedback ^= StateOdd[2] & (uint8_t) (LFSR_MASK_ODD >> 16);

    Feedback ^= Feedback >> 4;
    Feedback ^= Feedback >> 2;
    Feedback ^= Feedback >> 1;

    /* Now the shifting of the Crypto1 state gets more complicated when
    * split up into even/odd parts. After some hard thinking, one can
    * see that after one LFSR clock cycle
    * - the new even state becomes the old odd state
    * - the new odd state becomes the old even state right-shifted by 1.
    * For shifting the even state, we convert it into a 32 bit int first */
    uint32_t Temp = 0;
    Temp |= ((uint32_t) StateEven[0] << 0);
    Temp |= ((uint32_t) StateEven[1] << 8);
    Temp |= ((uint32_t) StateEven[2] << 16);

    /* Proceed LFSR. Try to force compiler not to shift the unneded upper bits. */
    Temp = (Temp >> 1) & 0x00FFFFFF;

    /* Calculate MSBit of even state as input bit to LFSR */
    if ( (Feedback & 0x01) ^ In ) {
        Temp |= (uint32_t) 1 << (8 * LFSR_SIZE/2 - 1);
    }

    /* Convert even state back into byte array and swap odd/even state
    * as explained above. */
    StateEven[0] = StateOdd[0];
    StateEven[1] = StateOdd[1];
    StateEven[2] = StateOdd[2];

    StateOdd[0] = (uint8_t) (Temp >> 0);
    StateOdd[1] = (uint8_t) (Temp >> 8);
    StateOdd[2] = (uint8_t) (Temp >> 16);
}

uint8_t Crypto1FilterOutput(void) {
    /* Calculate the functions fa, fb.
    * Note that only bits {4...23} of the odd state
    * get fed into these function.
    * The tables are designed to hold mask values, which
    * can simply be ORed together to produce the resulting
    * 5 bits that are used to lookup the output bit.
    */
    uint8_t Sum = 0;

    Sum |= TableAB[0][(StateOdd[0] >> 4) & 0x0F];
    Sum |= TableAB[1][(StateOdd[1] >> 0) & 0x0F];
    Sum |= TableAB[2][(StateOdd[1] >> 4) & 0x0F];
    Sum |= TableAB[3][(StateOdd[2] >> 0) & 0x0F];
    Sum |= TableAB[4][(StateOdd[2] >> 4) & 0x0F];

    return TableC[Sum];
}

void Crypto1Setup(uint8_t Key[6], uint8_t Uid[4], uint8_t CardNonce[4])
{
    uint8_t i;

    /* Again, one trade off when splitting up the state into even/odd parts
    * is that loading the key into the state becomes a little more difficult.
    * The inner loop generates 8 even and 8 odd bits from 16 key bits and
    * the outer loop stores them. */
    for (i=0; i<(LFSR_SIZE/2); i++) {
        uint8_t EvenByte = 0;
        uint8_t OddByte = 0;
        uint16_t KeyWord = ((uint16_t) Key[2*i+1] << 8) | Key[2*i+0];
        uint8_t j;

        for (j=0; j<8; j++) {
            EvenByte >>= 1;
            OddByte >>= 1;

            if (KeyWord & (1<<0)) {
                EvenByte |= 0x80;
            }

            if (KeyWord & (1<<1)) {
                OddByte |= 0x80;
            }

            KeyWord >>= 2;
        }

        StateEven[i] = EvenByte;
        StateOdd[i] = OddByte;
    }

    /* Use Uid XOR CardNonce as feed-in and do 32 clocks on the
    * Crypto1 LFSR.*/
    uint32_t Temp = 0;

    Temp |= (uint32_t) (Uid[0] ^ CardNonce[0]) << 0;
    Temp |= (uint32_t) (Uid[1] ^ CardNonce[1]) << 8;
    Temp |= (uint32_t) (Uid[2] ^ CardNonce[2]) << 16;
    Temp |= (uint32_t) (Uid[3] ^ CardNonce[3]) << 24;

    for (i=0; i<32; i++) {
        uint8_t Out = Crypto1FilterOutput();

        Crypto1LFSR(Temp & 0x01);
        Temp >>= 1;

        /* Store the keystream for later use */
        if (Out) {
            Temp |= (uint32_t) 1 << 31;
        }
    }

    /* Crypto1 state register is now set up to be used for authentication.
    * In case of nested authentication, we need to use the produced keystream
    * to encrypt the CardNonce. For this case we do the encryption in-place. */
    CardNonce[0] ^= (uint8_t) (Temp >> 0);
    CardNonce[1] ^= (uint8_t) (Temp >> 8);
    CardNonce[2] ^= (uint8_t) (Temp >> 16);
    CardNonce[3] ^= (uint8_t) (Temp >> 24);
}

void Crypto1Auth(uint8_t EncryptedReaderNonce[4])
{
    uint32_t Temp = 0;

    /* For ease of processing, we convert the encrypted reader nonce
    * into a 32 bit integer */
    Temp |= (uint32_t) EncryptedReaderNonce[0] << 0;
    Temp |= (uint32_t) EncryptedReaderNonce[1] << 8;
    Temp |= (uint32_t) EncryptedReaderNonce[2] << 16;
    Temp |= (uint32_t) EncryptedReaderNonce[3] << 24;

    uint8_t i;

    for (i=0; i<32; i++) {
        /* Decrypt one output bit of the given encrypted nonce using the
        * filter output as keystream. */
        uint8_t Out = Crypto1FilterOutput();
        uint8_t Bit = Out ^ (Temp & 0x01);

        /* Feed back the bit to load the LFSR with the (decrypted) nonce */
        Crypto1LFSR(Bit);
        Temp >>= 1;
    }
}

uint8_t Crypto1Byte(void)
{
    uint8_t KeyStream = 0;
    uint8_t i;

    /* Generate 8 keystream-bits */
    for (i=0; i<8; i++) {

        /* Calculate output of function-network and cycle LFSR with no
        * additional input, thus linearly! */
        uint8_t Out = Crypto1FilterOutput();
        Crypto1LFSR(0);

        /* Store keystream bit */
        KeyStream >>= 1;

        if (Out) {
            KeyStream |= (1<<7);
        }
    }

    return KeyStream;
}

uint8_t Crypto1Nibble(void)
{
    uint8_t KeyStream = 0;
    uint8_t i;

    /* Generate 4 keystream-bits */
    for (i=0; i<4; i++) {

        /* Calculate output of function-network and cycle LFSR with no
        * additional input, thus linearly! */
        uint8_t Out = Crypto1FilterOutput();
        Crypto1LFSR(0);

        /* Store keystream bit */
        KeyStream >>= 1;

        if (Out) {
            KeyStream |= (1<<3);
        }
    }

    return KeyStream;
}

void Crypto1PRNG(uint8_t State[4], uint16_t ClockCount)
{
    while(ClockCount--) {
        /* Actually, the PRNG is a 32 bit register with the upper 16 bit
        * used as a LFSR. Furthermore only mask-byte 2 contains feedback at all.
        * We rely on the compiler to optimize this for us here.
        * XOR all tapped bits to a single feedback bit. */
        uint8_t Feedback = 0;

        Feedback ^= State[0] & (uint8_t) (PRNG_MASK >> 0);
        Feedback ^= State[1] & (uint8_t) (PRNG_MASK >> 8);
        Feedback ^= State[2] & (uint8_t) (PRNG_MASK >> 16);
        Feedback ^= State[3] & (uint8_t) (PRNG_MASK >> 24);

        Feedback ^= Feedback >> 4;
        Feedback ^= Feedback >> 2;
        Feedback ^= Feedback >> 1;

        /* For ease of processing convert the state into a 32 bit integer first */
        uint32_t Temp = 0;

        Temp |= (uint32_t) State[0] << 0;
        Temp |= (uint32_t) State[1] << 8;
        Temp |= (uint32_t) State[2] << 16;
        Temp |= (uint32_t) State[3] << 24;

        /* Cycle LFSR and feed back. */
        Temp >>= 1;

        if (Feedback & 0x01) {
            Temp |= (uint32_t) 1 << (8 * PRNG_SIZE - 1);
        }

        /* Store back state */
        State[0] = (uint8_t) (Temp >> 0);
        State[1] = (uint8_t) (Temp >> 8);
        State[2] = (uint8_t) (Temp >> 16);
        State[3] = (uint8_t) (Temp >> 24);
    }


}
