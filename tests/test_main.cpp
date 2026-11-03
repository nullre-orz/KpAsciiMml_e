#define BOOST_TEST_MODULE MusicComTests
#include <boost/test/included/unit_test.hpp>
#include "test_support.h"

BOOST_AUTO_TEST_SUITE(Parser)

BOOST_AUTO_TEST_CASE(MmlSyntaxAndBoundaries)
{
    Fixtures fixtures;
    TestRunner runner("Parser");
    RunParserChannelTests(runner, fixtures);
    RunParserNoteTests(runner, fixtures);
    RunParserControlTests(runner, fixtures);
    RunParserEffectTests(runner, fixtures);
    RunParserDirectRegisterTests(runner, fixtures);
    RunParserLoopTests(runner, fixtures);
    RunParserStrTests(runner, fixtures);
    RunParserSSGEnvTests(runner, fixtures);
    RunParserErrorTests(runner, fixtures);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Registers)

BOOST_AUTO_TEST_CASE(SsgWrites)
{
    Fixtures fixtures;
    TestRunner runner("Registers");
    RunRegisterTests(runner, fixtures);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(MusicComFacade)

BOOST_AUTO_TEST_CASE(TransactionalLoad)
{
    Fixtures fixtures;
    TestRunner runner("MusicComFacade");
    RunMusicComTests(runner, fixtures);
    RunPluginTests(runner, fixtures);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(DataModel)

BOOST_AUTO_TEST_CASE(DefaultsAndConversions)
{
    Fixtures fixtures;
    TestRunner runner("DataModel");
    RunDataTests(runner, fixtures);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(SoundData)

BOOST_AUTO_TEST_CASE(ParserAndIterator)
{
    Fixtures fixtures;
    TestRunner runner("SoundData");
    RunSoundDataTests(runner, fixtures);
    RunSoundParserErrorTests(runner, fixtures);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Sequencer)

BOOST_AUTO_TEST_CASE(MusicComCompatibility)
{
    Fixtures fixtures;
    TestRunner runner("Sequencer");
    RunSequencerTests(runner, fixtures);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Behavior)

BOOST_AUTO_TEST_CASE(StateTransitions)
{
    Fixtures fixtures;
    TestRunner runner("Behavior");
    RunBehaviorTests(runner, fixtures);
}

BOOST_AUTO_TEST_SUITE_END()
