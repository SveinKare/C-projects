# Use the official Maven image with Amazon Corretto 21
FROM maven:3.9.6-amazoncorretto-21 as build
WORKDIR /app
COPY pom.xml .
COPY src ./src
# Package the application
RUN mvn clean package -DskipTests

# If you also need a specific Java runtime, use the corresponding Corretto image
FROM amazoncorretto:21
COPY --from=build /app/target/backend-1.0.jar /app.jar
ENTRYPOINT ["java","-jar","/app.jar"]

