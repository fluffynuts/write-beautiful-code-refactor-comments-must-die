/* defined somewhere, perhaps at the top of the cpp file? */
#define SERVICE_BITS 16
#define CRC_BITS 4
#define TAIL_BITS 6
#define BITS_PER_BYTE 8
/* resume class fragment */

std::vector<std::complex<double>> plcp_data::EncodePLPCFrame(Rate rate, std::vector<unsigned char> payload, unsigned short service)
{
    RateParams rate_params = RateParams(rate);

    int num_symbols = this.CalculateNumberOfSymbolsFor(&payload, &rate_params);

    int num_data_bits = this.CalculateDateBitsFor(&rate_params, num_symbols);
    int num_data_bytes = num_data_bits / BITS_PER_BYTE;

    int num_pad_bits = this.CalculatePaddingBitsFor(&payload, num_data_bits);

    std::vector<unsigned char> data = this.Join(&service, &payload, num_data_bytes);

    this.SetCRCOn(&data, &payload);
    this.Scramble(num_data_bytes, &data);
    this.ConvolutionallyEncode(num_data_bytes, &data);

    // Puncture the data  (this comment is redundant -- the method describes what they does)
    std::vector<unsigned char> data_punctured = puncturer::puncture(data_encoded, rate_params);

    this.Interleave(&data_punctured);

    // Modulate the data  (this comment is redundant -- the method describes what it does)
    std::vector<std::complex<double>> data_modulated = modulator::modulate(data_interleaved, rate);
    
    return data_modulated;
}

std::vector<unsigned char> plcp_data::Interleave(std::vector<unsigned char> *data)
{
    interleaver inter;
    return inter.interleave(*data);
}


void plcp_data::ConvolutionallyEncode(int num_data_bits, std::vector<unsigned char> *data_ptr)
{
    std::vector<unsigned char> data = (*data_ptr);  // because my pointers are rusty
    std::vector<unsigned char> data_encoded(num_data_bits * 2, 0);  // TODO: #define the magic number 2
    viterbi v;
    v.conv_encode(&data[0], data_encoded.data(), num_data_bits-6);  // TODO: #define the magic number 6
}

void plcp_data::Scramble(int num_data_bytes, std::vector<unsigned char> *data)
{
    std::vector<unsigned char> scrambled(num_data_bytes+1, 0);
    int state = 93, feedback = 0;
    for(int x = 0; x < num_data_bytes; x++)
    {
       feedback = (!!(state & 64)) ^ (!!(state & 8));
       scrambled[x] = feedback ^ (*data)[x];
       state = ((state << 1) & 0x7E) | feedback;
    }
    data->swap(scrambled);
}

void plcp_data::SetCRCOn(std::vector<unsigned char> *data_vector, std::vector<unsigned char> *payload)
{
    boost::crc_32_type crc;
    std::vector<unsigned char> data = *data_vector; // because I'm not sure my pointers don't suck
    crc.process_bytes(&data[0], 2 + payload->size());
    unsigned int calculated_crc = crc.checksum();
    memcpy(&data[2 + payload->size()], &calculated_crc, 4); // TODO: factor out magic value 4 into constant
}

std::vector<unsigned char> plcp_data::Join(unsigned short *service, std::vector<unsigned char> *payload, int num_data_bytes)
{
    std::vector<unsignedchar> data(num_data_bytes + 1, 0);
    memcpy(&data, service, 2);
    memcpy(&data, payload->data(), payload->size());
    return data;
}

int plcp_data::CalculatePaddingBitsFor(std::vector<unsigned char> *payload, int num_data_bits)
{
    return num_data_bits - (SERVICE_BITS * (payload->size() + CRC_BITS) + TAIL_BITS)
}

int plcp_data::CalculateNumberOfSymbolsFor(std::vector<unsigned char> *payload, RateParams *rate_params)
{
    return std::ceil(
            double((SERVICE_BITS + BITS_PER_BYTE * (payload->size() + CRC_BITS) + TAIL_BITS)) /
            double(rate_params->dbps));
}

int plcp_data::CalculateDataBitsFor(RateParams &rate_params, int num_symbols)
{
    return num_symbols * rate_params->dbps;
}
