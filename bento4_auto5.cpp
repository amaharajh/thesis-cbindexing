#include <iostream>

#include "Ap4.h"
#include "Ap4SampleDescription.h"

#include "link.h"
#include <string>
#include <vector>
#include <math.h>

//Define global variables here
AP4_ByteStream* inputByteStream;
AP4_ByteStream* outputByteStream;
AP4_SyntheticSampleTable* videoMdataSynSampleTable;
AP4_SyntheticSampleTable* audioMdataSynSampleTable;
int videoSampleTableOffset = 0;
int audioSampleTableOffset = 0;
bool vidInitialize = false;
char* input_filename = 0;
char* output_filename = 0;
AP4_UI64 audioDuration = 0;
int step1 = 0;
int total1 = 0;
double audioMs = 0.0;

std::vector<unsigned __int16> audioVector;
double audioxi = 0;
double audioxiu = 0;
unsigned __int16 audioMean = 0;
unsigned __int16 audioVar = 0;
int audioSampleTracker = 0;
int audioSampleTraining = 1600;
bool audioFlag = false;
bool audioDetection = false;

//Inherited Processor class with member variables and functions
class MyProcessor : public AP4_Processor
{
public:

	//Progress Listener class and Function
	MyProcessor():AP4_Processor(){};
	
	class MyProgressListener : public ProgressListener
	{
	public:
		AP4_Result OnProgress(unsigned int step, unsigned int total);
	};

	AP4_Result Initialize(AP4_AtomParent& top_level, 
						  AP4_ByteStream& stream,
						  MyProgressListener* listener = NULL);

	AP4_Result Finalize(AP4_AtomParent& top_level,
						MyProgressListener* listener = NULL);

	//Fragment Handler class and Function
	class MyFragmentHandler : public FragmentHandler
	{
	public:
		AP4_Result ProcessSample(AP4_DataBuffer& data_in, AP4_DataBuffer& data_out);
	};

	//Track Handler Class and Function
	class MyVideoTrackHandler : public TrackHandler
	{
	public:
		MyVideoTrackHandler():TrackHandler(){}
		AP4_Result ProcessTrack();
		AP4_Result ProcessSample(AP4_DataBuffer& data_in, AP4_DataBuffer& data_out);
	};

	class MyAudioTrackHandler : public TrackHandler
	{
	public:
		MyAudioTrackHandler():TrackHandler(){}
		AP4_Result ProcessTrack();
		AP4_Result ProcessSample(AP4_DataBuffer& data_in, AP4_DataBuffer& data_out);
	};

	//Overriding CreateTrackHandler and CreateFragmentHandler functions from AP4_Processor
	TrackHandler* CreateTrackHandler(AP4_TrakAtom* trak);
	MyFragmentHandler* CreateFragmentHandler(AP4_TrakAtom* trak);
};

//Finished defining class member variables and functions. Defining functions now.

AP4_Result MyProcessor::MyProgressListener::OnProgress(unsigned int step, unsigned int total)
{
	printf("\r%d/%d", step, total);
	step1 = step;
	total1 = total;
	return AP4_SUCCESS;
}

AP4_Result MyProcessor::Initialize(AP4_AtomParent& top_level, AP4_ByteStream& stream, MyProgressListener* listener){
	return AP4_SUCCESS;
}


AP4_Result MyProcessor::MyFragmentHandler::ProcessSample(AP4_DataBuffer& data_in, AP4_DataBuffer& data_out)
{
	//data_out.SetBuffer(data_in.GetData, data_in.GetDataSize);
	data_out.SetData(data_in.GetData(), data_in.GetDataSize());
	return AP4_SUCCESS;
}

MyProcessor::TrackHandler* MyProcessor::CreateTrackHandler(AP4_TrakAtom* trak){
	//If we need to return multiple types of track handleres, then check trak atoms and use switch case or if else statements
	//return new MyTrackHandler();

	//At this point, we are not concerned with the metadata handler so create new handlers for video and audio ONLY
	AP4_ContainerAtom* mp4v = AP4_DYNAMIC_CAST(AP4_ContainerAtom, trak->FindChild("mdia/minf/stbl/stsd/mp4v"));
	AP4_ContainerAtom* mp4a = AP4_DYNAMIC_CAST(AP4_ContainerAtom, trak->FindChild("mdia/minf/stbl/stsd/mp4a"));
	if (mp4v != NULL) {
		MyProcessor::MyVideoTrackHandler* videoTrackHandler = new MyProcessor::MyVideoTrackHandler();
		return videoTrackHandler;
	}
	else if (mp4a != NULL) {
		MyProcessor::MyAudioTrackHandler* audioTrackHandler = new MyProcessor::MyAudioTrackHandler();
		audioDuration = trak->GetDuration();
		return audioTrackHandler;
	}
	else{
		return NULL;
	}

	return NULL;
}

AP4_Result MyProcessor::MyVideoTrackHandler::ProcessTrack(){
	//Create synthetic sample table
	videoMdataSynSampleTable = new AP4_SyntheticSampleTable();

	//Create sample description
	AP4_SampleDescription* videoMdataSampleDescr = new AP4_SampleDescription(AP4_SampleDescription::TYPE_MPEG, AP4_SAMPLE_FORMAT_TEXT, NULL);

	//Add sample description
	videoMdataSynSampleTable->AddSampleDescription(videoMdataSampleDescr);
	return AP4_SUCCESS;
}

AP4_Result MyProcessor::MyVideoTrackHandler::ProcessSample(AP4_DataBuffer &data_in, AP4_DataBuffer &data_out){

	if (vidInitialize == false){
		initializeCodeBook(input_filename);
		vidInitialize = true;
	}

	int vidTime = runCodeBook();
	if (vidTime != NULL){

		std::string videoParseResult = videoParser(vidTime);

		//Put string into memory byte stream
		AP4_Result mdataResult;

		AP4_MemoryByteStream* videoMdataStream = new AP4_MemoryByteStream(videoParseResult.size());
		mdataResult = videoMdataStream->WriteString(videoParseResult.c_str());
		if (AP4_FAILED(mdataResult)){
			fprintf(stderr, "ERROR: Cannot write metadata to byte stream \n");
			return EXIT_FAILURE;
		}

		//Now that resultant XML string is in memory byte stream, it is considered as a sample and we need to add it to the sample table

		//Create size to track sample size
		AP4_LargeSize sample_size;
		videoMdataStream->GetSize(sample_size);

		//Add samples to synthetic sample table
		mdataResult = videoMdataSynSampleTable->AddSample(*videoMdataStream,		//data stream	
			videoSampleTableOffset,													//offset
			(AP4_Size)sample_size,													//size
			3500,								//inputAP4Movie->GetDuration(),		//duration, this is the time where the detection took place
			0,									//sampleTableIndex,					//Sample description index, typically 0
			0,																		//dts, may have to calculate this
			0,																		//cts_delta, may have to calculate this
			true);																	//sync

		if (AP4_FAILED(mdataResult)) {
			fprintf(stderr, "ERROR: cannot add sample \n");
			return EXIT_FAILURE;
		}

		//Calculate new data offset
		videoSampleTableOffset = videoSampleTableOffset + (int)sample_size;
	}
	
	//Echo data_in to data_out using the line below
	data_out.SetData(data_in.GetData(), data_in.GetDataSize());

	return AP4_SUCCESS;
}

AP4_Result MyProcessor::MyAudioTrackHandler::ProcessTrack(){
	//Create synthetic sample table
	audioMdataSynSampleTable = new AP4_SyntheticSampleTable();

	//Create sample description
	AP4_SampleDescription* audioMdataSampleDescr = new AP4_SampleDescription(AP4_SampleDescription::TYPE_MPEG, AP4_SAMPLE_FORMAT_TEXT, NULL);

	//Add sample description
	audioMdataSynSampleTable->AddSampleDescription(audioMdataSampleDescr);
	
	return AP4_SUCCESS;
}

AP4_Result MyProcessor::MyAudioTrackHandler::ProcessSample(AP4_DataBuffer& data_in, AP4_DataBuffer& data_out){
	
	//Grab data in unsigned 16-bit format for AAC. Also grab data size for training of 1600 samples
	unsigned __int16 *looknum = (unsigned __int16 *)(data_in.GetData());
	int sizelook = data_in.GetDataSize();

	//Check to see if more than 1600 samples have been used for training. If not, go ahead with training process
	if (audioFlag == false){
		if (audioSampleTracker < audioSampleTraining){
			for (int i = 0; i < sizelook; i++){
				//Store sample values in vector and calculate audioxi parameter here for mean
				audioVector.push_back(looknum[i]);
				audioxi += looknum[i];
			}
			
			//Compute new audioSampleTracker total
			audioSampleTracker += sizelook;
		}
		else{
			
			//Calculate mean
			audioMean = audioxi / audioSampleTracker;

			//Calculate variance parameter here
			for (std::vector<unsigned __int16>::iterator i = audioVector.begin(); i != audioVector.end(); i++){
				audioxiu += (*i - audioMean) * (*i - audioMean);
			}

			//Calculate variance here. This is really standard deviation
			audioVar = sqrt((double)audioxiu/audioSampleTracker);

			//Trigger flag to not execute this section of code again and to begin execution of silence detection
			audioFlag = true;
			audioDetection = true;
		}
	}

	double avg1 = 0;

	//Since training complete, can now conduct silence detection

	if (audioDetection == true){
		//double avg1 = 0;
		for (int i = 0; i < sizelook; i++){
			avg1 += looknum[i];
		}
		avg1 = avg1 / sizelook;

		double resultTest1 = avg1 - audioMean;
		resultTest1 = abs(resultTest1);

		if ((double)resultTest1/audioVar > 0.3173){

			//Calculate result in milliseconds and pass to parser
			audioMs = (double)step1 / total1 * audioDuration;
			
			std::string parseAudioResult = audioParser(audioMs);
			
			//Put string into audio memory byte stream
			AP4_Result mdataResult;

			AP4_MemoryByteStream* audioMdataStream = new AP4_MemoryByteStream(parseAudioResult.size());
			mdataResult = audioMdataStream->WriteString(parseAudioResult.c_str());
			if (AP4_FAILED(mdataResult)){
				fprintf(stderr, "ERROR: Cannot write metadata to byte stream \n");
				return EXIT_FAILURE;
			}

			//Create size to track sample size
			AP4_LargeSize sample_size;
			audioMdataStream->GetSize(sample_size);

			//Add samples to synthetic sample table
			mdataResult = audioMdataSynSampleTable->AddSample(*audioMdataStream,		//data stream	
				audioSampleTableOffset,													//offset
				(AP4_Size)sample_size,													//size
				500,								//inputAP4Movie->GetDuration(),		//duration, this is the time where the detection took place
				0,									//sampleTableIndex,					//Sample description index, typically 0
				0,																		//dts, may have to calculate this
				0,																		//cts_delta, may have to calculate this
				true);																	//sync

			if (AP4_FAILED(mdataResult)) {
				fprintf(stderr, "ERROR: cannot add sample \n");
				return EXIT_FAILURE;
			}

			//Calculate new data offset
			audioSampleTableOffset = audioSampleTableOffset + (int)sample_size;
		}
		
	}

	//Echo data_int to data_out using line below
	data_out.SetData(data_in.GetData(), data_in.GetDataSize());
	return AP4_SUCCESS;
}

AP4_Result MyProcessor::Finalize(AP4_AtomParent& top_level, MyProgressListener* listener){
	releaseMemory();
	return AP4_SUCCESS;
}

AP4_Result AddMetadataTracks(){
	//Now that the samples have been processed into the sample table, we just need to add the sample table to the new metadata track

	//Grab AP4 Movie object to get parameters
	AP4_File* inputAP4File = new AP4_File(*inputByteStream);
	AP4_Movie* inputAP4Movie = inputAP4File->GetMovie();

	//Add samples to video metadata track
	AP4_Track* videoMdataTrack = new AP4_Track( AP4_Track::TYPE_TEXT,		//Track type 
		videoMdataSynSampleTable,				//Sample table for track
		4,										//Track ID
		inputAP4Movie->GetTimeScale(),			//Movie Time Scale
		inputAP4Movie->GetDuration(),			//Track Duration
		inputAP4Movie->GetTimeScale(),			//25,										//inputMovie->GetTimeScale(), should be mediaTimeScale, can use movieTimeScale if no other choice
		inputAP4Movie->GetDuration(),			//2296										//inputMovie->GetDuration(), should be mediaDuration, can use MovieDuration if no other choice
		"eng",									//language
		NULL,									//width
		NULL);									//height

	//Add samples to audio metadata track
	AP4_Track* audioMdataTrack = new AP4_Track( AP4_Track::TYPE_TEXT,
		audioMdataSynSampleTable,
		5,
		inputAP4Movie->GetTimeScale(),
		inputAP4Movie->GetDuration(),
		inputAP4Movie->GetTimeScale(),
		inputAP4Movie->GetDuration(),
		"eng",
		NULL,
		NULL);

	//Add tracks to movie
	inputAP4Movie->AddTrack(videoMdataTrack);
	inputAP4Movie->AddTrack(audioMdataTrack);

	//If possible without having to write movie, can we convert back movie to a file byte stream?
	AP4_File *NewTrackFile = new AP4_File(inputAP4Movie);

	//Write movie
	AP4_FileWriter::Write(*inputAP4File, *outputByteStream);

	return AP4_SUCCESS;
}



int main (int argc, char* argv[]){

	AP4_Result result;

#pragma region inputs

	//Create ByteStreams for process
	inputByteStream = NULL;
	//input_filename = "poors1.mp4";
	//output_filename = "output_video.mp4";
	
	input_filename = argv[1];
	output_filename = argv[2];
	
	//filename = "poors_ex_s2.mp4";
	//result = AP4_FileByteStream::Create("input_2tracks.mp4", AP4_FileByteStream::STREAM_MODE_READ, inputByteStream);

	result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, inputByteStream);
	if (AP4_FAILED(result)) {
		fprintf(stderr, "ERROR: cannot open input file \n");
		return EXIT_FAILURE;
	}

	outputByteStream = NULL;
	//result = AP4_FileByteStream::Create("output_video.mp4", AP4_FileByteStream::STREAM_MODE_WRITE, outputByteStream);
	
	result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, outputByteStream);
	if (AP4_FAILED(result)) {
		fprintf(stderr, "ERROR: cannot open output file \n");
		return EXIT_FAILURE;
	}

#pragma endregion inputs

	//FragmentHandler* fragmenter = new FragmentHandler();
	MyProcessor::MyProgressListener* listener = new MyProcessor::MyProgressListener();
	AP4_AtomFactory& atom_factory = AP4_DefaultAtomFactory::Instance;


	//Construct Processor Instance
	MyProcessor* processor2 = new MyProcessor();

	//Run Process
	processor2->Process(*inputByteStream, *outputByteStream, listener, atom_factory);

	//Run my function here to add metadata tracks
	result = AddMetadataTracks();
	
	if (AP4_FAILED(result)){
		fprintf(stderr, "ERROR: Cannot add metadata tracks \n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}