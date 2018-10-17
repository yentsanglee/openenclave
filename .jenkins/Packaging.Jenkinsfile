pipeline {
  agent any
  stages {
stage('Build, Test, and Package') {
      parallel {
        stage('SGX1FLC Package Debug') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1FLC -b Debug -d --build_package'
             
          withCredentials([usernamePassword(credentialsId: '40060061-6050-40f7-ac6a-53aeb767245f', passwordVariable: 'SERVICE_PRINCIPAL_PASSWORD', usernameVariable: 'SERVICE_PRINCIPAL_ID')]) {
            sh 'bash ./scripts/deploy-docs build ssh'
          }   
        
        }
      }
    }
  }
}
}
