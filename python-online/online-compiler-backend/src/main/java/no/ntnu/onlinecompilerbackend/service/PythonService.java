package no.ntnu.onlinecompilerbackend.service;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import org.springframework.stereotype.Service;

@Service
public class PythonService {
  public String executePythonCode(String code) {
    code = code.replaceAll("\"", "'");
    ProcessBuilder processBuilder = new ProcessBuilder("python", "-c", code);
    processBuilder.redirectErrorStream(true);
    try {
      Process process = processBuilder.start();

      StringBuilder output = new StringBuilder();
      BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
      String line;
      while ((line = reader.readLine()) != null) {
        output.append(line).append(String.format("%n"));
      }
      process.waitFor();
      return output.toString();
    } catch (Exception e) {
      return e.getMessage();
    }
  }
}
