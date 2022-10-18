#include <Tasks.h>
#include <due_can.h>

#define INO_FILE

#include "jen.hpp"

#include "pid.hpp"
#include "bounding_helper.hpp"
#include "rm_motor.hpp"

#define AIR_1A 3
#define AIR_1B 2
#define AIR_2A 4
#define AIR_2B 5

CAN_FRAME tx_msg, rx_msg;

#define GROUP1_MOTOR_COUNT 4
RMM3508Motor group1_rm[GROUP1_MOTOR_COUNT] = {
  RMM3508Motor(0, SPEED_PID_MODE), 
  RMM3508Motor(1, SPEED_PID_MODE),
  RMM3508Motor(2, SPEED_PID_MODE),
  RMM3508Motor(3, SPEED_PID_MODE)
  };

DECLARE_WATCHER(JsonObject, carbase_setting, "rc.s",
  group1_rm[0].set_speed_pid(value["m0"]["pid"]);
  group1_rm[1].set_speed_pid(value["m1"]["pid"]);
  group1_rm[2].set_speed_pid(value["m2"]["pid"]);
  group1_rm[3].set_speed_pid(value["m3"]["pid"]);

  static int count = 0;
  console << "updated pid" << count++ << "\n";
)

DECLARE_WATCHER(JsonArray, gen_output, "rg.o",
  bool air1 = value[0].as<bool>();
  bool air2 = value[1].as<bool>();

  digitalWrite(AIR_1A, air1 ? LOW : HIGH);
  digitalWrite(AIR_1B, air1 ? HIGH : LOW);
  digitalWrite(AIR_2A, air2 ? LOW : HIGH);
  digitalWrite(AIR_2B, air2 ? HIGH : LOW);

  static int count = 0;
  // console << "updated " << count++ << "\n";
  console << count++ << "\n";
)

DECLARE_WATCHER(JsonArray, carbase_output, "rc.o",
  int rm0 = value[0].as<int>();
  int rm1 = value[1].as<int>();
  int rm2 = value[2].as<int>();
  int rm3 = value[3].as<int>();

  group1_rm[0].target_tick = rm0;
  group1_rm[1].target_tick = rm1;
  group1_rm[2].target_tick = rm2;
  group1_rm[3].target_tick = rm3;

  // static int count = 0;
  // console << "shooter updated " << count++ << "\n";
  // console << x << "\n";
)

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(AIR_1A, OUTPUT);
  pinMode(AIR_1B, OUTPUT);
  pinMode(AIR_2A, OUTPUT);
  pinMode(AIR_2B, OUTPUT);

  START_WATCHER(carbase_setting);
  START_WATCHER(gen_output);
  START_WATCHER(carbase_output);

  Can0.begin(CAN_BPS_1000K);  //  For communication with RM motors

  gb.setup(115200);

  Tasks_Add((Task)loop1, 1, 0);
  Tasks_Add((Task)loop2, 10, 0);
  Tasks_Add((Task)loop3, 1, 0);

  // Start task scheduler
  Tasks_Start();
}

void loop1() {  // Serial
  gb.loop();
}

void loop2() {  // Send sensors / encoders data
  StaticJsonDocument<128> gen_feedback;
  gen_feedback[0] = 123;
  gen_feedback[1] = 456;

  gb.write("rg.f", gen_feedback);

  StaticJsonDocument<128> carbase_feedback;
  carbase_feedback[0]  = group1_rm[0].unbound_tick;
  carbase_feedback[1]  = group1_rm[0].speed;
  carbase_feedback[2]  = group1_rm[0].output

  carbase_feedback[3]  = group1_rm[1].unbound_tick;
  carbase_feedback[4]  = group1_rm[1].speed;
  carbase_feedback[5]  = group1_rm[1].output;

  carbase_feedback[6]  = group1_rm[2].unbound_tick;
  carbase_feedback[7]  = group1_rm[2].speed;
  carbase_feedback[8]  = group1_rm[2].output;

  carbase_feedback[9]  = group1_rm[3].unbound_tick;
  carbase_feedback[10] = group1_rm[3].speed;
  carbase_feedback[11] = group1_rm[3].output;

  gb.write("rc.f", carbase_feedback);
}

void loop3() {  // PID Calculation
  tx_msg.id = 0x200;
  tx_msg.length = 8;

  for (int i = 0; i < GROUP1_MOTOR_COUNT; i++) {
    short output = group1_rm[i].get_output();
    tx_msg.data.byte[i * 2] = output >> 8;
    tx_msg.data.byte[i * 2 + 1] = output;
  }

  Can0.sendFrame(tx_msg);
}

// the loop function runs over and over again forever, runs ~14000 times per second
void loop() {
  Can0.watchFor();
  Can0.read(rx_msg);

  for (int i = 0; i < GROUP1_MOTOR_COUNT; i++) {
    if (group1_rm[i].handle_packet(rx_msg.id, rx_msg.data.byte)) break;
  }
}