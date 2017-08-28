#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include <iostream>
#include <string>
#include <sstream>

int vidIndex = 0;
int audioIndex = 0;

std::string time_to_string(int millis){

	//Declare hour, minutes, seconds, milliseconds, remainder integers
	int hour(0), minutes(0), seconds(0), millisecs(0), remainder(0);
	std::string hour_string, minutes_string, seconds_string, millisecs_string;
	std::stringstream convert;

	//Begin conversion
	hour = millis / (3600 * 1000);
	remainder = millis % (3600 * 1000);

	minutes = remainder / (60 * 1000);
	remainder = remainder % (60 * 1000);

	seconds = remainder / 1000;
	millisecs = remainder % 1000;

	//Run conversion into string format
	convert << hour;
	hour_string = convert.str();
	convert.str("");
	if (hour_string.length() < 2){
		hour_string = "0" + hour_string;
	}

	convert << minutes;
	minutes_string = convert.str();
	convert.str("");
	if (minutes_string.length() < 2){
		minutes_string = "0" + minutes_string;
	}

	convert << seconds;
	seconds_string = convert.str();
	convert.str("");
	if (seconds_string.length() < 2){
		seconds_string = "0" + seconds_string;
	}

	convert<<millisecs;
	millisecs_string = convert.str();
	convert.str("");
	if (millisecs_string.length() == 3){
		millisecs_string = millisecs_string.substr(0, millisecs_string.size() - 1);
	}
	else if (millisecs_string.length() == 1){
		millisecs_string = "0" + millisecs_string;
	}

	std::string result_string;
	result_string = hour_string + ":" + minutes_string + ":" + seconds_string + ":" + millisecs_string;

	return result_string;
}

std::string videoParser(int time){

	//Create XML Data Object
	rapidxml::xml_document<> doc;

	//Create index for tracking of vidID
	vidIndex++;
	std::ostringstream convert;
	convert << vidIndex;
	std::string vidID = "C" + convert.str();
	
	//XML Declaration
	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

#pragma region generalHeader
	//Starting general header stuff

	//Root node for MPEG-7
	rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "Mpeg7");
	root->append_attribute(doc.allocate_attribute("xmlns", "urn:mpeg:mpeg7:schema:2001"));
	root->append_attribute(doc.allocate_attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance"));
	root->append_attribute(doc.allocate_attribute("xsi:schemaLocation", "urn:mpeg:mpeg7:schema:2001 CDPschemaFromMP7P11.xsd"));
	doc.append_node(root);

	//Child node declaring description profile
	rapidxml::xml_node<>* child = doc.allocate_node(rapidxml::node_element, "DescriptionProfile");
	child->append_attribute(doc.allocate_attribute("profileAndLevelIndication", "urn:mpeg:mpeg7:profiles:2004:CDP"));
	root->append_node(child);

#pragma endregion generalHeader

#pragma region ChapterInsertion
	//Parsing Video cuepoint XML data
	rapidxml::xml_node<>* chapterDescr = doc.allocate_node(rapidxml::node_element, "Description");
	chapterDescr->append_attribute(doc.allocate_attribute("xsi:type", "Content Entity Type"));
	root->append_node(chapterDescr);

	rapidxml::xml_node<>* multimediaContent = doc.allocate_node(rapidxml::node_element, "MultimediaContent");
	multimediaContent->append_attribute(doc.allocate_attribute("xsi:type", "VideoType"));
	chapterDescr->append_node(multimediaContent);

	rapidxml::xml_node<>* videoID = doc.allocate_node(rapidxml::node_element, "Video id");
	videoID->append_attribute(doc.allocate_attribute(NULL, "Insert Name of Video here"));
	multimediaContent->append_node(videoID);

	rapidxml::xml_node<>* temporalDecomp = doc.allocate_node(rapidxml::node_element, "TemporalDecomposition");
	videoID->append_node(temporalDecomp);

	rapidxml::xml_node<>* videoSegment = doc.allocate_node(rapidxml::node_element, "VideoSegment");
	videoSegment->append_attribute(doc.allocate_attribute("id", vidID.c_str()));
	temporalDecomp->append_node(videoSegment);

	rapidxml::xml_node<>* textAnnoCN = doc.allocate_node(rapidxml::node_element, "TextAnnotation");
	textAnnoCN->append_attribute(doc.allocate_attribute("type", "CodeBookEntry"));
	videoSegment->append_node(textAnnoCN);

	rapidxml::xml_node<>* freeTextAnnoCN = doc.allocate_node(rapidxml::node_element, "FreeTextAnnotation");
	freeTextAnnoCN->value("Put chapter title here");
	textAnnoCN->append_node(freeTextAnnoCN);

	rapidxml::xml_node<>* textAnnoCS = doc.allocate_node(rapidxml::node_element, "TextAnnotation");
	textAnnoCS->append_attribute(doc.allocate_attribute("type", "ClipSynopsis"));
	videoSegment->append_node(textAnnoCS);

	rapidxml::xml_node<>* freeTextAnnoCS = doc.allocate_node(rapidxml::node_element, "FreeTextAnnotation");
	freeTextAnnoCS->value("Start Annotation");
	textAnnoCS->append_node(freeTextAnnoCS);

	rapidxml::xml_node<>* mediaTime = doc.allocate_node(rapidxml::node_element, "MediaTime");
	videoSegment->append_node(mediaTime);

	//Run time conversion here and convert to string format for XML parsing
	std::string mpeg_time_point = time_to_string(time);

	rapidxml::xml_node<>* mediaTimePoint = doc.allocate_node(rapidxml::node_element, "MediaTimePoint");
	
	//Convert mpeg_time_point string to c_str() format
	mediaTimePoint->value(mpeg_time_point.c_str());
	mediaTime->append_node(mediaTimePoint);

#pragma endregion ChapterInsertion

	//Store as string using rapidxml::print
	std::string xml_as_string;
	rapidxml::print(std::back_inserter(xml_as_string), doc);

	return xml_as_string;
}

std::string audioParser(int time){

	//Create XML Data Object
	rapidxml::xml_document<> doc;

	//Create index for tracking of vidID
	audioIndex++;
	std::ostringstream convert;
	convert << audioIndex;
	std::string audioID = "C" + convert.str();
	
	//XML Declaration
	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

#pragma region generalHeader2
	//Starting general header stuff

	//Root node for MPEG-7
	rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "Mpeg7");
	root->append_attribute(doc.allocate_attribute("xmlns", "urn:mpeg:mpeg7:schema:2001"));
	root->append_attribute(doc.allocate_attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance"));
	root->append_attribute(doc.allocate_attribute("xsi:schemaLocation", "urn:mpeg:mpeg7:schema:2001 CDPschemaFromMP7P11.xsd"));
	doc.append_node(root);

	//Child node declaring description profile
	rapidxml::xml_node<>* child = doc.allocate_node(rapidxml::node_element, "DescriptionProfile");
	child->append_attribute(doc.allocate_attribute("profileAndLevelIndication", "urn:mpeg:mpeg7:profiles:2004:CDP"));
	root->append_node(child);

#pragma endregion generalHeader2

#pragma region ChapterInsertion2
	//Parsing Video cuepoint XML data
	rapidxml::xml_node<>* chapterDescr = doc.allocate_node(rapidxml::node_element, "Description");
	chapterDescr->append_attribute(doc.allocate_attribute("xsi:type", "Content Entity Type"));
	root->append_node(chapterDescr);

	rapidxml::xml_node<>* multimediaContent = doc.allocate_node(rapidxml::node_element, "MultimediaContent");
	multimediaContent->append_attribute(doc.allocate_attribute("xsi:type", "AudioType"));
	chapterDescr->append_node(multimediaContent);

	rapidxml::xml_node<>* videoID = doc.allocate_node(rapidxml::node_element, "Audio id");
	videoID->append_attribute(doc.allocate_attribute(NULL, "Insert Name of Audio here"));
	multimediaContent->append_node(videoID);

	rapidxml::xml_node<>* temporalDecomp = doc.allocate_node(rapidxml::node_element, "TemporalDecomposition");
	videoID->append_node(temporalDecomp);

	rapidxml::xml_node<>* audioSegment = doc.allocate_node(rapidxml::node_element, "AudioSegment");
	audioSegment->append_attribute(doc.allocate_attribute("id", audioID.c_str()));
	temporalDecomp->append_node(audioSegment);

	rapidxml::xml_node<>* textAnnoCN = doc.allocate_node(rapidxml::node_element, "TextAnnotation");
	textAnnoCN->append_attribute(doc.allocate_attribute("type", "Silence"));
	audioSegment->append_node(textAnnoCN);

	rapidxml::xml_node<>* freeTextAnnoCN = doc.allocate_node(rapidxml::node_element, "FreeTextAnnotation");
	freeTextAnnoCN->value("Put chapter title here");
	textAnnoCN->append_node(freeTextAnnoCN);

	rapidxml::xml_node<>* textAnnoCS = doc.allocate_node(rapidxml::node_element, "TextAnnotation");
	textAnnoCS->append_attribute(doc.allocate_attribute("type", "ClipSynopsis"));
	audioSegment->append_node(textAnnoCS);

	rapidxml::xml_node<>* freeTextAnnoCS = doc.allocate_node(rapidxml::node_element, "FreeTextAnnotation");
	freeTextAnnoCS->value("Start Annotation");
	textAnnoCS->append_node(freeTextAnnoCS);

	rapidxml::xml_node<>* mediaTime = doc.allocate_node(rapidxml::node_element, "MediaTime");
	audioSegment->append_node(mediaTime);

	//Run time conversion here and convert to string format for XML parsing
	std::string mpeg_time_point = time_to_string(time);

	rapidxml::xml_node<>* mediaTimePoint = doc.allocate_node(rapidxml::node_element, "MediaTimePoint");
	
	//Convert mpeg_time_point string to c_str() format
	mediaTimePoint->value(mpeg_time_point.c_str());
	mediaTime->append_node(mediaTimePoint);

#pragma endregion ChapterInsertion2

	//Store as string using rapidxml::print
	std::string xml_as_string;
	rapidxml::print(std::back_inserter(xml_as_string), doc);

	return xml_as_string;
}