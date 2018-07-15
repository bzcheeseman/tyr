//
// Created by Aman LaChapelle on 7/14/18.
//
// tyr
// Copyright (c) 2018 Aman LaChapelle
// Full license at tyr/LICENSE.txt
//

/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#include "Router.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

void tyr::runtime::Router::SetRouteModule(std::string serialized) {
  llvm::SMDiagnostic smd_err;
  m_route_module_ = llvm::parseIR(*llvm::MemoryBuffer::getMemBuffer(serialized),
                                  smd_err, m_ctx_, true);
  if (!m_route_module_) {
    smd_err.print("SetExecModule", llvm::errs(), true);
  }

  const std::string target_triple = llvm::sys::getDefaultTargetTriple();
  const std::string cpu = llvm::sys::getHostCPUName();
  llvm::StringMap<bool> feats_map;
  llvm::sys::getHostCPUFeatures(feats_map);

  std::string features = "";
  for (auto &entry : feats_map) {
    if (entry.second)
      features += "+" + std::string(entry.first()) + ",";
  }

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
      target->createTargetMachine(target_triple, cpu, features, options, RM);

  m_route_module_->setDataLayout(target_machine->createDataLayout());
  m_route_module_->setTargetTriple(target_triple);
}

std::string tyr::runtime::Router::RouteCall(std::string path,
                                            std::string args) {
  return std::string(); // call LLVM JIT on the module to route it according to
                        // rules
}

void tyr::runtime::Router::ServeRouter(const std::string &base_address,
                                       uint16_t port) {

  int s;
  sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(base_address.c_str());

  // Bind socket
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    llvm::errs() << "Unable to create socket";
    exit(EXIT_FAILURE);
  }

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    llvm::errs() << "Unable to bind";
    exit(EXIT_FAILURE);
  }

  if (listen(s, 1) < 0) {
    llvm::errs() << "Unable to listen";
    exit(EXIT_FAILURE);
  }

  // Listen to incoming requests and route based on specific paths
  while (m_should_continue_) {
    //    struct sockaddr_in addr;
    //    uint len = sizeof(addr);
    //    SSL *ssl;
    //    const char reply[] = "test\n";
    //
    //    int client = accept(sock, (struct sockaddr*)&addr, &len);
    //    if (client < 0) {
    //      perror("Unable to accept");
    //      exit(EXIT_FAILURE);
    //    }
    //
    //    ssl = SSL_new(ctx);
    //    SSL_set_fd(ssl, client);
    //
    //    if (SSL_accept(ssl) <= 0) {
    //      ERR_print_errors_fp(stderr);
    //    }
    //    else {
    //      SSL_write(ssl, reply, strlen(reply));
    //    }
    //
    //    SSL_free(ssl);
    //    close(client);
  }

  close(s);
  // Listen on socket
}
