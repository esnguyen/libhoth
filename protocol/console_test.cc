// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "protocol/console.h"

#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include "protocol/status.h"
#include "test/libhoth_device_mock.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;

TEST_F(LibHothTest, console_get_channel_status_success) {
  struct hoth_channel_status_response exp_resp = {
      .write_offset = 0x12345678,
  };

  EXPECT_CALL(mock_, send(_,
                          UsesCommand(HOTH_CMD_BOARD_SPECIFIC_BASE +
                                      HOTH_PRV_CMD_HOTH_CHANNEL_STATUS),
                          _))
      .WillOnce(Return(LIBHOTH_OK));

  EXPECT_CALL(mock_, receive)
      .WillOnce(
          DoAll(CopyResp(&exp_resp, sizeof(exp_resp)), Return(LIBHOTH_OK)));

  struct libhoth_htool_console_opts opts = {
      .channel_id = EROT_CHANNEL_ID,
  };
  uint32_t offset = 0;
  EXPECT_EQ(libhoth_get_channel_status(&hoth_dev_, &opts, &offset),
            HOTH_SUCCESS);
  EXPECT_EQ(offset, 0x12345678);
}

TEST_F(LibHothTest, console_get_channel_status_null_params) {
  uint32_t offset = 0;
  struct libhoth_htool_console_opts opts = {};

  libhoth_error err = libhoth_get_channel_status(nullptr, &opts, &offset);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_get_channel_status(&hoth_dev_, nullptr, &offset);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_get_channel_status(&hoth_dev_, &opts, nullptr);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);
}

TEST_F(LibHothTest, console_read_success) {
  struct {
    struct hoth_channel_read_response resp;
    char data[16];
  } exp_resp = {
      .resp = {.offset = 100},
      .data = "hello world\n",
  };

  EXPECT_CALL(mock_, send(_,
                          UsesCommand(HOTH_CMD_BOARD_SPECIFIC_BASE +
                                      HOTH_PRV_CMD_HOTH_CHANNEL_READ),
                          _))
      .WillOnce(Return(LIBHOTH_OK));

  EXPECT_CALL(mock_, receive)
      .WillOnce(
          DoAll(CopyResp(&exp_resp, sizeof(exp_resp)), Return(LIBHOTH_OK)));

  int pipefd[2];
  ASSERT_EQ(pipe(pipefd), 0);

  uint32_t offset = 100;
  EXPECT_EQ(libhoth_read_console(&hoth_dev_, pipefd[1], false, EROT_CHANNEL_ID,
                                 &offset),
            HOTH_SUCCESS);
  EXPECT_EQ(offset, 100 + sizeof(exp_resp.data));

  char read_buf[32] = {0};
  int n = read(pipefd[0], read_buf, sizeof(read_buf));
  EXPECT_EQ(n, sizeof(exp_resp.data));
  EXPECT_STREQ(read_buf, "hello world\n");

  close(pipefd[0]);
  close(pipefd[1]);
}

TEST_F(LibHothTest, console_read_null_params) {
  uint32_t offset = 0;
  libhoth_error err =
      libhoth_read_console(nullptr, 1, false, EROT_CHANNEL_ID, &offset);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_read_console(&hoth_dev_, 1, false, EROT_CHANNEL_ID, nullptr);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);
}

TEST_F(LibHothTest, console_get_uart_config_success) {
  struct hoth_channel_uart_config exp_resp = {
      .baud_rate = 115200,
      .reserved = 0,
  };

  EXPECT_CALL(mock_,
              send(_,
                   UsesCommand(HOTH_CMD_BOARD_SPECIFIC_BASE +
                               HOTH_PRV_CMD_HOTH_CHANNEL_UART_CONFIG_GET),
                   _))
      .WillOnce(Return(LIBHOTH_OK));

  EXPECT_CALL(mock_, receive)
      .WillOnce(
          DoAll(CopyResp(&exp_resp, sizeof(exp_resp)), Return(LIBHOTH_OK)));

  struct libhoth_htool_console_opts opts = {
      .channel_id = EROT_CHANNEL_ID,
  };
  struct hoth_channel_uart_config resp = {};
  EXPECT_EQ(libhoth_get_uart_config(&hoth_dev_, &opts, &resp), HOTH_SUCCESS);
  EXPECT_EQ(resp.baud_rate, 115200);
}

TEST_F(LibHothTest, console_get_uart_config_null_params) {
  struct libhoth_htool_console_opts opts = {};
  struct hoth_channel_uart_config resp = {};

  libhoth_error err = libhoth_get_uart_config(nullptr, &opts, &resp);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_get_uart_config(&hoth_dev_, nullptr, &resp);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_get_uart_config(&hoth_dev_, &opts, nullptr);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);
}

TEST_F(LibHothTest, console_set_uart_config_success) {
  EXPECT_CALL(mock_,
              send(_,
                   UsesCommand(HOTH_CMD_BOARD_SPECIFIC_BASE +
                               HOTH_PRV_CMD_HOTH_CHANNEL_UART_CONFIG_SET),
                   _))
      .WillOnce(Return(LIBHOTH_OK));

  EXPECT_CALL(mock_, receive)
      .WillOnce(DoAll(CopyResp("", 0), Return(LIBHOTH_OK)));

  struct libhoth_htool_console_opts opts = {
      .channel_id = EROT_CHANNEL_ID,
  };
  struct hoth_channel_uart_config config = {
      .baud_rate = 57600,
      .reserved = 0,
  };
  EXPECT_EQ(libhoth_set_uart_config(&hoth_dev_, &opts, &config), HOTH_SUCCESS);
}

TEST_F(LibHothTest, console_set_uart_config_null_params) {
  struct libhoth_htool_console_opts opts = {};
  struct hoth_channel_uart_config config = {};

  libhoth_error err = libhoth_set_uart_config(nullptr, &opts, &config);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_set_uart_config(&hoth_dev_, nullptr, &config);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_set_uart_config(&hoth_dev_, &opts, nullptr);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);
}

TEST_F(LibHothTest, console_write_null_params) {
  bool quit = false;
  libhoth_error err =
      libhoth_write_console(nullptr, EROT_CHANNEL_ID, false, &quit);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);

  err = libhoth_write_console(&hoth_dev_, EROT_CHANNEL_ID, false, nullptr);
  EXPECT_EQ(LIBHOTH_ERR_GET_CTX(err), HOTH_CTX_CMD_EXEC);
  EXPECT_EQ(LIBHOTH_ERR_GET_SPACE(err), HOTH_HOST_SPACE_LIBHOTH);
  EXPECT_EQ(LIBHOTH_ERR_GET_CODE(err), LIBHOTH_ERR_INVALID_PARAMETER);
}
