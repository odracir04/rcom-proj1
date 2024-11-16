# Data Link Layer Protocol Implementation for RS-232 File Transfer

## Overview

This project implements a Data Link Layer Protocol for file transfer between two computers connected via an RS-232 serial cable. The implementation includes a transmitter/receiver application, providing a complete solution to transfer a file stored on a computer hard disk. The protocol is implemented in C for a Linux environment.
 
## Features

- Data Link Layer Protocol: Custom implementation of the data link layer with byte stuffing, retransmission mechanism and timeout robustness.
- Transmitter and Receiver Applications: Simple applications to test file transfer over RS-232.
- API for Upper Layers: The data link layer protocol exposes functions for use by higher layers.
- Asynchronous Communication: Fully supports RS-232 asynchronous serial communication.

## Course Information

This project is part of the Computer Networks course, designed to provide hands-on experience with low-level networking concepts and protocol implementation.
