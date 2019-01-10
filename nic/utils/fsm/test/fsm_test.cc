#include <gtest/gtest.h>
#include "nic/utils/fsm/fsm.hpp"
#include <map>
#include "lib/twheel/twheel.hpp"
#include "nic/hal/test/utils/hal_test_utils.hpp"
#include <vector>

using namespace std::placeholders;
using hal::utils::fsm_state_t;
using hal::utils::fsm_state_ctx;
using hal::utils::fsm_event_data;
using hal::utils::fsm_transition_t;
using hal::utils::fsm_transition_func;
using hal::utils::fsm_state_func;
using hal::utils::fsm_state_machine_t;
using hal::utils::fsm_state_machine_def_t;
using hal::utils::fsm_timer_t;
using hal::utils::fsm_state_timer_ctx;
using sdk::lib::twheel;

#define STATE_ID_1 1
#define STATE_ID_2 2
#define STATE_ID_3 3
#define STATE_ID_4 4


#define TIMEOUT_EVENT 255
#define REMOVE_EVENT 254

class fsm_test : public ::testing::Test {
 protected:
  fsm_test() {}

  virtual ~fsm_test() {}

  // will be called immediately after the constructor before each test
  virtual void SetUp() {}

    std::vector<string> results;
    fsm_transition_func transition_handler(const string &result,
                                           bool success = true) {
        return [=](fsm_state_ctx, fsm_event_data) {
            this->results.push_back(result);
            if (success) {
                return true;
            } else {
                return false;
            }
        };
    }
    fsm_state_func entry_exit_handler(const string &result) {
        return [=](fsm_state_ctx) { this->results.push_back(result); };
    }
    // will be called immediately after each test before the destructor
    virtual void TearDown() {}

    // Will be called at the beginning of all test cases in this class
    static void SetUpTestCase() {
      hal_test_utils_slab_disable_delete();
    }
};

fsm_state_machine_def_t *global_sm = nullptr;
fsm_timer_t *global_timer = nullptr;

fsm_state_machine_def_t *get_sm_func() { return global_sm; }
fsm_timer_t *get_timer_func() { return global_timer; }
TEST_F(fsm_test, fsm_init) {
  // clang-format off
        FSM_SM_BEGIN(my_sm)
            FSM_STATE_BEGIN(STATE_ID_1, 1, NULL, NULL)
                FSM_TRANSITION(1, NULL, STATE_ID_1)
                FSM_TRANSITION(2, NULL, STATE_ID_1)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_2, 1, NULL, NULL)
                FSM_TRANSITION(1, NULL, STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_2)
            FSM_STATE_END
        FSM_SM_END
    // clang-format on
        global_sm = &my_sm;
        fsm_state_machine_t sm = fsm_state_machine_t(get_sm_func, 1, 2, TIMEOUT_EVENT, REMOVE_EVENT);
        ASSERT_EQ(my_sm.size(), 2);
        ASSERT_EQ(sm.get_state(), STATE_ID_1);
        ASSERT_EQ(sm.get_def(), &my_sm);
}

TEST_F(fsm_test, fsm_one_transition) {
    // clang-format off
        FSM_SM_BEGIN(my_sm)
            FSM_STATE_BEGIN(STATE_ID_1, 1, entry_exit_handler("E1"),
                    entry_exit_handler("E2"))
                FSM_TRANSITION(1, transition_handler("s1"), STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_1)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_2, 1, entry_exit_handler("E3"),
                    entry_exit_handler("E4"))
                FSM_TRANSITION(1, NULL, STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_2)
            FSM_STATE_END
        FSM_SM_END
    // clang-format on
        global_sm = &my_sm;
        fsm_state_machine_t sm = fsm_state_machine_t(get_sm_func, 1, 2, TIMEOUT_EVENT, REMOVE_EVENT);
        sm.process_event(1, NULL);
        ASSERT_EQ(sm.get_state(), STATE_ID_2);
        EXPECT_EQ(results, std::vector<string>({"E1", "s1", "E2", "E3"}));
}

TEST_F(fsm_test, fsm_one_transition_fail) {
    // clang-format off
        FSM_SM_BEGIN(my_sm)
            FSM_STATE_BEGIN(STATE_ID_1, 1, NULL, NULL)
                FSM_TRANSITION(1, transition_handler("s1", false), STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_1)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_2, 1, NULL, NULL)
                FSM_TRANSITION(1, NULL, STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_2)
            FSM_STATE_END
        FSM_SM_END
        global_sm = &my_sm;
        fsm_state_machine_t sm = fsm_state_machine_t(get_sm_func, 1, 3, TIMEOUT_EVENT, REMOVE_EVENT);
        sm.process_event(1, NULL);
        ASSERT_EQ(sm.get_state(), STATE_ID_1);
        EXPECT_EQ(results, std::vector<string>({"s1"}));
}

TEST_F(fsm_test, fsm_one_transition_end) {
    // clang-format off
        FSM_SM_BEGIN(my_sm)
            FSM_STATE_BEGIN(STATE_ID_1, 1, NULL, NULL)
                FSM_TRANSITION(1, transition_handler("s1", true), STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_1)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_2, 1, NULL, NULL)
            FSM_STATE_END
        FSM_SM_END
    // clang-format on
        global_sm = &my_sm;
        fsm_state_machine_t sm = fsm_state_machine_t(get_sm_func, 1, 2, TIMEOUT_EVENT, REMOVE_EVENT);
        sm.process_event(1, NULL);
        ASSERT_EQ(sm.get_state(), STATE_ID_2);
        EXPECT_EQ(results, std::vector<string>({"s1"}));
        ASSERT_TRUE(sm.state_machine_competed());
}

TEST_F(fsm_test, fsm_multiple_transitions) {
    // clang-format off
        FSM_SM_BEGIN(my_sm)
            FSM_STATE_BEGIN(STATE_ID_1, 1, entry_exit_handler("S1_ENTRY"),
                    entry_exit_handler("S1_EXIT"))
                FSM_TRANSITION(1, transition_handler("F1"), STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_1)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_2, 1, entry_exit_handler("S2_ENTRY"),
                    entry_exit_handler("S2_EXIT"))
                FSM_TRANSITION(2, transition_handler("F2"), STATE_ID_3)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_3, 1, entry_exit_handler("S3_ENTRY"),
                    entry_exit_handler("S3_EXIT"))
                FSM_TRANSITION(3, transition_handler("F3"), STATE_ID_4)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_4, 1, entry_exit_handler("S4_ENTRY"),
                    entry_exit_handler("S4_EXIT"))
            FSM_STATE_END
        FSM_SM_END
            // clang-format on
        global_sm = &my_sm;
        fsm_state_machine_t sm = fsm_state_machine_t(get_sm_func, 1, 4,
                TIMEOUT_EVENT, REMOVE_EVENT);
        sm.process_event(1, NULL);
        sm.process_event(2, NULL);
        sm.process_event(3, NULL);
        ASSERT_EQ(sm.get_state(), STATE_ID_4);
        EXPECT_EQ(results,
                  std::vector<string>({"S1_ENTRY", "F1", "S1_EXIT", "S2_ENTRY",
                                       "F2", "S2_EXIT", "S3_ENTRY", "F3",
                                       "S3_EXIT", "S4_ENTRY"}));
        ASSERT_TRUE(sm.state_machine_competed());
}

TEST_F(fsm_test, fsm_one_transition_with_throw) {
    // clang-format off
        FSM_SM_BEGIN(my_sm)
            FSM_STATE_BEGIN(STATE_ID_1, 1, entry_exit_handler("E1"),
                    entry_exit_handler("E2"))
                FSM_TRANSITION(1, transition_handler("s1"), STATE_ID_2)
                FSM_TRANSITION(1, transition_handler("s1"), STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_1)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_2, 1, entry_exit_handler("E3"),
                    entry_exit_handler("E4"))
                FSM_TRANSITION(1, NULL, STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_2)
            FSM_STATE_END
        FSM_SM_END
          // clang-format on
            global_sm = &my_sm;
        fsm_state_machine_t sm = fsm_state_machine_t(get_sm_func, 1, 2,
                TIMEOUT_EVENT, REMOVE_EVENT);
          sm.process_event(1, NULL);
          ASSERT_EQ(sm.get_state(), STATE_ID_2);
          EXPECT_EQ(results, std::vector<string>({"E1", "s1", "E2", "E3"}));
}

class fsm_test_app : public ::testing::Test {
   public:
    class TestApplication {
       public:
        fsm_state_machine_t* sm;
        class TestFsm;
        static TestFsm* fsm;

        class TestFsm {
           public:
            fsm_state_machine_def_t sm_def;
            void set_state_machine(fsm_state_machine_def_t sm_def) {
                this->sm_def = sm_def;
        }
        TestFsm() { _init_state_machine(); }
        void _init_state_machine() {
            // clang-format off
                    FSM_SM_BEGIN((sm_def))
                        FSM_STATE_BEGIN(STATE_ID_1, 1, NULL, NULL)
                            FSM_TRANSITION(1, SM_BIND_NON_STATIC(
                                    TestFsm, state_1_event_1_function),
                                    STATE_ID_2)
                            FSM_TRANSITION(2, SM_BIND_NON_STATIC(
                                    TestFsm, state_1_event_2_function),
                                    STATE_ID_2)
                            FSM_TRANSITION(3, SM_BIND_NON_STATIC(
                                    TestFsm, state_1_event_3_function),
                                    STATE_ID_4)
                        FSM_STATE_END
                        FSM_STATE_BEGIN(STATE_ID_2, 1, NULL, NULL)
                            FSM_TRANSITION(1, SM_BIND_NON_STATIC(
                                    TestFsm, state_2_event_1_function),
                                    STATE_ID_2)
                            FSM_TRANSITION(2, SM_BIND_NON_STATIC(
                                    TestFsm, state_2_event_2_function),
                                    STATE_ID_3)
                            FSM_TRANSITION(3, SM_BIND_NON_STATIC(
                                    TestFsm, state_2_event_3_function),
                                    STATE_ID_2)
                        FSM_STATE_END
                        FSM_STATE_BEGIN(STATE_ID_3, 1, NULL, NULL)
                            FSM_TRANSITION(1, SM_BIND_NON_STATIC(
                                    TestFsm, state_3_event_1_function),
                                    STATE_ID_4)
                            FSM_TRANSITION(2, SM_BIND_NON_STATIC(
                                    TestFsm, state_3_event_2_function),
                                    STATE_ID_4)
                            FSM_TRANSITION(3, SM_BIND_NON_STATIC(
                                    TestFsm, state_3_event_3_function),
                                    STATE_ID_4)
                        FSM_STATE_END
                            FSM_STATE_BEGIN(STATE_ID_4, 1, NULL, NULL)
                       FSM_STATE_END
                    FSM_SM_END
              this->set_state_machine(sm_def);
          }
        public:
        bool state_1_event_1_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }
        bool state_1_event_2_function(fsm_state_ctx ctx, fsm_event_data data) {
            TestApplication *app_trans = reinterpret_cast<TestApplication *>(ctx);
            app_trans->sm->throw_event(3, data);
            // Make sure no transition.
            return false;
        }
        bool state_1_event_3_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }
        bool state_2_event_1_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }
        bool state_2_event_2_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }
        bool state_2_event_3_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }
        bool state_3_event_1_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }
        bool state_3_event_2_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }
        bool state_3_event_3_function(fsm_state_ctx ctx, fsm_event_data data) {
            return true;
        }

        };

        static fsm_state_machine_def_t* get_sm_func() {
            return &fsm->sm_def;
        }

        TestApplication() {
            this->sm = nullptr;
            // this->sm_def = NULL;
            this->sm = new fsm_state_machine_t(get_sm_func, STATE_ID_1,
                    STATE_ID_4, TIMEOUT_EVENT, REMOVE_EVENT, (fsm_state_ctx)this);
        }




        void process_event(uint32_t event, fsm_event_data data) {
            this->sm->process_event(event, data);
        }

    };
    TestApplication* fsm_app;

   protected:
    fsm_test_app() {
        this->fsm_app = new TestApplication();
    }

    virtual ~fsm_test_app() {}

    // will be called immediately after the constructor before each test
    virtual void SetUp() {}

    // will be called immediately after each test before the destructor
    virtual void TearDown() {}
};

fsm_test_app::TestApplication::TestFsm *fsm_test_app::TestApplication::fsm = \
        new fsm_test_app::TestApplication::TestFsm();

TEST_F(fsm_test_app, fsm_test_app_throw_transition) {
    ASSERT_EQ(this->fsm_app->sm->get_state(), STATE_ID_1);
    this->fsm_app->process_event(2, nullptr);
    ASSERT_EQ(this->fsm_app->sm->get_state(), STATE_ID_4);
}

TEST_F(fsm_test_app, fsm_test_app_one_transition) {
    ASSERT_EQ(this->fsm_app->sm->get_state(), STATE_ID_1);
    this->fsm_app->process_event(1, nullptr);
    ASSERT_EQ(this->fsm_app->sm->get_state(), STATE_ID_2);
    this->fsm_app->process_event(2, nullptr);
    ASSERT_EQ(this->fsm_app->sm->get_state(), STATE_ID_3);
    this->fsm_app->process_event(1, nullptr);
    ASSERT_EQ(this->fsm_app->sm->get_state(), STATE_ID_4);

}



class TestTimer: public fsm_timer_t {
    twheel   *test_twheel;

   public:
    explicit TestTimer(uint32_t id) : fsm_timer_t(1){
    test_twheel = twheel::factory(10, 100, false);
    }

    virtual fsm_state_timer_ctx add_timer(uint64_t timeout, fsm_state_machine_t *ctx,
            bool periodic = false) {
        void *timer = test_twheel->add_timer(1, timeout, (void*)ctx,
                timeout_handler, periodic);
        return reinterpret_cast<fsm_state_timer_ctx>(timer);
    }
    virtual void delete_timer(fsm_state_timer_ctx*) {

    }

    virtual uint64_t get_timeout_remaining(fsm_state_timer_ctx timer)
    {
        return test_twheel->get_timeout_remaining(timer) / TIME_MSECS_PER_SEC ;
    }

    static void
    timeout_handler(void *timer, uint32_t timer_id, void *ctxt)
    {
        fsm_state_machine_t* sm_ = reinterpret_cast<fsm_state_machine_t*>(ctxt);
        //100 is timeout event
        sm_->process_event(100, NULL);
    }

    virtual void delete_timer(fsm_state_timer_ctx)
    {

    }
    ~TestTimer() {

    }

    void tick(uint32_t msecs_elapsed) {
        this->test_twheel->tick(msecs_elapsed);
    }
};

TEST_F(fsm_test, fsm_test_timeout) {

    TestTimer * timer = new TestTimer(1);
    // clang-format off
        FSM_SM_BEGIN(my_sm)
            FSM_STATE_BEGIN(STATE_ID_1, 1000, entry_exit_handler("E1"),
                    entry_exit_handler("E2"))
                FSM_TRANSITION(1, transition_handler("s1"), STATE_ID_2)
                FSM_TRANSITION(1, transition_handler("s1"), STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_1)
                FSM_TRANSITION(100, NULL, STATE_ID_2)
            FSM_STATE_END
            FSM_STATE_BEGIN(STATE_ID_2, 2000, entry_exit_handler("E3"),
                    entry_exit_handler("E4"))
                FSM_TRANSITION(1, NULL, STATE_ID_2)
                FSM_TRANSITION(2, NULL, STATE_ID_2)
            FSM_STATE_END
        FSM_SM_END
    // clang-format on
        global_sm = &my_sm;
        global_timer = timer;
        fsm_state_machine_t sm = fsm_state_machine_t(get_sm_func, 1, 2,
                TIMEOUT_EVENT, REMOVE_EVENT, NULL, get_timer_func);
        sleep(1);
        timer->tick(1000 + 100);
        ASSERT_EQ(sm.get_state(), STATE_ID_2);
        EXPECT_EQ(results, std::vector<string>({"E1", "E2", "E3"}));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

