package no.ntnu.onlinecompilerbackend.controller;

import no.ntnu.onlinecompilerbackend.model.Request;
import no.ntnu.onlinecompilerbackend.model.Response;
import no.ntnu.onlinecompilerbackend.service.PythonService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class PythonController {
  private final PythonService pythonService;

  @Autowired
  public PythonController(PythonService pythonService) {
    this.pythonService = pythonService;
  }

  @PostMapping("/python")
  public ResponseEntity<Response> executePythonCode(@RequestBody Request request) {
    String output = pythonService.executePythonCode(request.getCode());
    Response response = new Response();
    response.setOutput(output);
    return ResponseEntity.ok(response);
  }
}
