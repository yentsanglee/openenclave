pipeline {
  agent any
  stages {
stage('Build, Test, and Package') {
      parallel {
        stage('SGX1FLC Package Debug') {

          steps {
            sh 'bash ./scripts/deploy-docs build ssh'
           }
        }
      }
    }
  }
}
