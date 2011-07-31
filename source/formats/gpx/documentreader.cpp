#include "documentreader.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/spirit/include/qi.hpp>

#include <powertabdocument/powertabdocument.h>
#include <powertabdocument/score.h>
#include <powertabdocument/guitar.h>
#include <powertabdocument/barline.h>
#include <powertabdocument/system.h>

Gpx::DocumentReader::DocumentReader(const std::string& xml)
{
    std::stringstream xmlStream;
    xmlStream << xml;
    read_xml(xmlStream, gpFile);
}

void Gpx::DocumentReader::readDocument(std::shared_ptr<PowerTabDocument> doc)
{
    readHeader(doc->GetHeader());
    readTracks(doc->GetGuitarScore());
    //readMasterBars();
    readBars();
    readVoices();
    readBeats();
    readRhythms();
    readNotes();
}

/// Loads the header information (song title, artist, etc)
void Gpx::DocumentReader::readHeader(PowerTabFileHeader& header)
{
    const ptree& gpHeader = gpFile.get_child("GPIF.Score");

    header.SetSongTitle(gpHeader.get<std::string>("Title"));
    header.SetSongArtist(gpHeader.get<std::string>("Artist"));
    header.SetSongAudioReleaseTitle(gpHeader.get<std::string>("Album"));
    header.SetSongLyricist(gpHeader.get<std::string>("Words"));
    header.SetSongComposer(gpHeader.get<std::string>("Music"));
    header.SetSongCopyright(gpHeader.get<std::string>("Copyright"));

    header.SetSongGuitarScoreTranscriber(gpHeader.get<std::string>("Tabber"));
    header.SetSongGuitarScoreNotes(gpHeader.get<std::string>("Instructions"));
}

namespace
{
/// Utility function for converting a string of space-separated values to a vector of that type
template <typename T>
void convertStringToList(const std::string& source, std::vector<T>& dest)
{
    using namespace boost::spirit::qi;

    bool parsed = parse(source.begin(), source.end(),
                        auto_ % ' ', dest);

    if (!parsed)
    {
        std::cerr << "Parsing of list failed!!" << std::endl;
    }
}
}

/// Imports the tracks and performs a conversion to the PowerTab Guitar class
void Gpx::DocumentReader::readTracks(Score *score)
{
    BOOST_FOREACH(const ptree::value_type& node, gpFile.get_child("GPIF.Tracks"))
    {
        const ptree& track = node.second;

        Score::GuitarPtr guitar = std::make_shared<Guitar>();

        guitar->SetDescription(track.get<std::string>("Name"));
        guitar->SetPreset(track.get<int>("GeneralMidi.Program"));

        guitar->SetInitialVolume(track.get("ChannelStrip.Volume", Guitar::DEFAULT_INITIAL_VOLUME));

        // Not all tracks will have a Properties node ...
        boost::optional<const ptree&> properties = track.get_child_optional("Properties");
        if (properties)
        {
            // Read the tuning - need to convert from a string of numbers separated by spaces to
            // a vector of integers
            const std::string tuningString = properties->get<std::string>("Property.Pitches");
            std::vector<uint8_t> tuningNotes;
            convertStringToList(tuningString, tuningNotes);

            guitar->GetTuning().SetTuningNotes(tuningNotes);

            // Read capo
            guitar->SetCapo(properties->get("Property.Fret", 0));
        }

        score->InsertGuitar(guitar);
    }
}

void Gpx::DocumentReader::readMasterBars()
{
    BOOST_FOREACH(const ptree::value_type& node, gpFile.get_child("GPIF.MasterBars"))
    {
        if (node.first != "MasterBar")
        {
            continue;
        }

        const ptree& masterBar = node.second;

        System::BarlinePtr barline = std::make_shared<Barline>();

        readKeySignature(masterBar, barline->GetKeySignature());
        readTimeSignature(masterBar, barline->GetTimeSignature());

        std::vector<int> barIds;
        convertStringToList(masterBar.get<std::string>("Bars"), barIds);
    }
}

void Gpx::DocumentReader::readBars()
{
    BOOST_FOREACH(const ptree::value_type& node, gpFile.get_child("GPIF.Bars"))
    {
        const ptree& currentBar = node.second;

        Gpx::Bar bar;
        bar.id = currentBar.get<int>("<xmlattr>.id");
        convertStringToList(currentBar.get<std::string>("Voices"), bar.voiceIds);

        bars[bar.id] = bar;
    }
}

void Gpx::DocumentReader::readKeySignature(const Gpx::DocumentReader::ptree& masterBar, KeySignature& key)
{
    key.SetKeyAccidentals(masterBar.get<uint8_t>("Key.AccidentalCount"));

    const std::string keyType = masterBar.get<std::string>("Key.Mode");

    if (keyType == "Major")
    {
        key.SetKeyType(KeySignature::majorKey);
    }
    else if (keyType == "Minor")
    {
        key.SetKeyType(KeySignature::minorKey);
    }
    else
    {
        std::cerr << "Unknown key type" << std::endl;
    }
}

void Gpx::DocumentReader::readTimeSignature(const Gpx::DocumentReader::ptree& masterBar,
                                            TimeSignature& timeSignature)
{
    const std::string timeString = masterBar.get<std::string>("Time");

    std::vector<int> timeSigValues;
    using namespace boost::spirit::qi;

    // import time signature (stored in text format - e.g. "4/4")
    bool parsed = parse(timeString.begin(), timeString.end(),
                        int_ >> '/' >> int_, timeSigValues);

    if (!parsed)
    {
        std::cerr << "Parsing of time signature failed!!" << std::endl;
    }
    else
    {
        timeSignature.SetMeter(timeSigValues[0], timeSigValues[1]);
    }
}

void Gpx::DocumentReader::readVoices()
{
    BOOST_FOREACH(const ptree::value_type& node, gpFile.get_child("GPIF.Voices"))
    {
        const ptree& currentVoice = node.second;

        Gpx::Voice voice;
        voice.id = currentVoice.get<int>("<xmlattr>.id");
        convertStringToList(currentVoice.get<std::string>("Beats"), voice.beatIds);

        voices[voice.id] = voice;
    }
}

void Gpx::DocumentReader::readBeats()
{
    BOOST_FOREACH(const ptree::value_type& node, gpFile.get_child("GPIF.Beats"))
    {
        const ptree& currentBeat = node.second;

        Gpx::Beat beat;
        beat.id = currentBeat.get<int>("<xmlattr>.id");
        beat.rhythmId = currentBeat.get<int>("Rhythm.<xmlattr>.ref");
        convertStringToList(currentBeat.get("Notes", ""), beat.noteIds);

        beats[beat.id] = beat;
    }
}

void Gpx::DocumentReader::readRhythms()
{
    BOOST_FOREACH(const ptree::value_type& node, gpFile.get_child("GPIF.Rhythms"))
    {
        const ptree& currentRhythm = node.second;

        Gpx::Rhythm rhythm;
        rhythm.id = currentRhythm.get<int>("<xmlattr>.id");

        const std::string noteValueStr = currentRhythm.get<std::string>("NoteValue");

        std::map<std::string, int> noteValuesToInt = {
            {"Whole", 1}, {"Half", 2}, {"Quarter", 4}, {"Eighth", 8},
            {"16th", 16}, {"32nd", 32}, {"64th", 64}
        };

        assert(noteValuesToInt.find(noteValueStr) != noteValuesToInt.end());
        rhythm.noteValue = noteValuesToInt.find(noteValueStr)->second;

        rhythms[rhythm.id] = rhythm;
    }
}

void Gpx::DocumentReader::readNotes()
{
    BOOST_FOREACH(const ptree::value_type& node, gpFile.get_child("GPIF.Notes"))
    {
        const ptree& currentNote = node.second;

        Gpx::Note note;
        note.id = currentNote.get<int>("<xmlattr>.id");
        note.properties = currentNote.get_child("Properties");

        notes[note.id] = note;
    }
}
